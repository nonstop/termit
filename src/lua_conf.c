/* Copyright © 2007-2016 Evgeny Ratnikov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <X11/Xlib.h> // XParseGeometry
#include <glib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "configs.h"
#include "keybindings.h"
#include "lua_api.h"

extern struct TermitData termit;
extern struct Configs configs;

lua_State* L = NULL;

void termit_lua_close()
{
    lua_close(L);
}

static void trace_menus(GArray* menus)
{
#ifdef DEBUG
    guint i = 0;
    for (; i<menus->len; ++i) {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        TRACE("%s items %d", um->name, um->items->len);
        guint j = 0;
        for (; j<um->items->len; ++j) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, j);
            TRACE("  %s: (%d) [%s]", umi->name, umi->lua_callback, umi->accel);
        }
    }
#endif
}

void termit_config_get_string(gchar** opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        if (*opt) {
            g_free(*opt);
        }
        *opt = g_strdup(lua_tostring(ls, index));
    }
}

void termit_config_get_double(double* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isnumber(ls, index)) {
        *opt = lua_tonumber(ls, index);
    }
}

void termit_config_getuint(guint* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isnumber(ls, index)) {
        *opt = lua_tointeger(ls, index);
    }
}

void termit_config_get_boolean(gboolean* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isboolean(ls, index)) {
        *opt = lua_toboolean(ls, index);
    }
}

void termit_config_get_function(int* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isfunction(ls, index)) {
        *opt = luaL_ref(ls, LUA_REGISTRYINDEX); // luaL_ref pops value so we restore stack size
        lua_pushnil(ls);
    }
}

void termit_config_get_color(GdkRGBA** opt, lua_State* ls, int index)
{
    gchar* color_str = NULL;
    termit_config_get_string(&color_str, ls, index);
    if (color_str) {
        GdkRGBA color = {};
        if (gdk_rgba_parse(&color, color_str) == TRUE) {
            *opt = gdk_rgba_copy(&color);
            TRACE("color_str=%s", color_str);
        }
    }
    g_free(color_str);
}

void termit_config_get_erase_binding(VteEraseBinding* opt, lua_State* ls, int index)
{
    gchar* str = NULL;
    termit_config_get_string(&str, ls, index);
    *opt = termit_erase_binding_from_string(str);
    g_free(str);
}

void termit_config_get_cursor_blink_mode(VteCursorBlinkMode* opt, lua_State* ls, int index)
{
    gchar* str = NULL;
    termit_config_get_string(&str, ls, index);
    *opt = termit_cursor_blink_mode_from_string(str);
    g_free(str);
}

void termit_config_get_cursor_shape(VteCursorShape* opt, lua_State* ls, int index)
{
    gchar* str = NULL;
    termit_config_get_string(&str, ls, index);
    *opt = termit_cursor_shape_from_string(str);
    g_free(str);
}

static void matchesLoader(const gchar* pattern, struct lua_State* ls, int index, void* data)
{
    TRACE("pattern=%s index=%d data=%p", pattern, index, data);
    if (!lua_isfunction(ls, index)) {
        ERROR("match [%s] without function: skipping", pattern);
        return;
    }
    GArray* matches = (GArray*)data;
    struct Match match = {};
    GError* err = NULL;
    match.regex = vte_regex_new_for_match(pattern, -1, VTE_REGEX_FLAGS_DEFAULT, &err);
    if (err) {
        TRACE("failed to compile regex [%s]: skipping", pattern);
        return;
    }
    match.flags = 0;
    match.pattern = g_strdup(pattern);
    termit_config_get_function(&match.lua_callback, ls, index);
    g_array_append_val(matches, match);
}

struct ColormapHelper
{
    GdkRGBA* colors;
    int idx;
};

static void colormapLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct ColormapHelper* ch = (struct ColormapHelper*)data;
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        const gchar* colorStr = lua_tostring(ls, index);
        if (!gdk_rgba_parse(&(ch->colors[ch->idx]), colorStr)) {
            ERROR("failed to parse color: %s %d - %s", name, ch->idx, colorStr);
        }
    } else {
        ERROR("invalid type in colormap: skipping");
    }
    ++ch->idx;
}

static void tabsLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    if (lua_istable(ls, index)) {
        GArray* tabs = (GArray*)data;
        struct TabInfo ti = {};
        if (termit_lua_load_table(ls, termit_lua_tab_loader, index, &ti)
                != TERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("failed to load tab: %s %s", name, lua_tostring(ls, 3));
        } else {
            g_array_append_val(tabs, ti);
        }
    } else {
        ERROR("unknown type instead if tab table: skipping");
        lua_pop(ls, 1);
    }
}

void termit_lua_load_colormap(lua_State* ls, int index, GdkRGBA** colors, glong* sz)
{
    if (lua_isnil(ls, index) || !lua_istable(ls, index)) {
        ERROR("invalid colormap type");
        return;
    }
    const int size = lua_rawlen(ls, index);
    if ((size != 8) && (size != 16) && (size != 24)) {
        ERROR("bad colormap length: %d", size);
        return;
    }
    struct ColormapHelper ch = {};
    ch.colors = g_malloc0(size * sizeof(GdkRGBA));
    if (termit_lua_load_table(ls, colormapLoader, index, &ch)
            == TERMIT_LUA_TABLE_LOADER_OK) {
        if (*colors) {
            g_free(*colors);
        }
        *colors = ch.colors;
        *sz = size;
    } else {
        ERROR("failed to load colormap");
        return;
    }
    TRACE("colormap loaded: size=%ld", *sz);
}

static void termit_config_get_position(GtkPositionType* pos, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        const char* str = lua_tostring(ls, index);
        if (strcmp(str, "Top") == 0) {
            *pos = GTK_POS_TOP;
        } else if (strcmp(str, "Bottom") == 0) {
            *pos = GTK_POS_BOTTOM;
        } else if (strcmp(str, "Left") == 0) {
            *pos = GTK_POS_LEFT;
        } else if (strcmp(str, "Right") == 0) {
            *pos = GTK_POS_RIGHT;
        } else {
            ERROR("unknown pos: [%s]", str);
        }
    }
}

void termit_lua_options_loader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct Configs* p_cfg = (struct Configs*)data;
    if (!strcmp(name, "tabName")) {
        termit_config_get_string(&(p_cfg->default_tab_name), ls, index);
    } else if (!strcmp(name, "windowTitle")) {
        termit_config_get_string(&(p_cfg->default_window_title), ls, index);
    } else if (!strcmp(name, "encoding")) {
        termit_config_get_string(&(p_cfg->default_encoding), ls, index);
    } else if (!strcmp(name, "wordCharExceptions")) {
        termit_config_get_string(&(p_cfg->default_word_char_exceptions), ls, index);
    } else if (!strcmp(name, "font")) {
        termit_config_get_string(&(p_cfg->style.font_name), ls, index);
    } else if (!strcmp(name, "foregroundColor")) {
        termit_config_get_color(&p_cfg->style.foreground_color, ls, index);
    } else if (!strcmp(name, "backgroundColor")) {
        termit_config_get_color(&p_cfg->style.background_color, ls, index);
    } else if (!strcmp(name, "showScrollbar")) {
        termit_config_get_boolean(&(p_cfg->show_scrollbar), ls, index);
    } else if (!strcmp(name, "fillTabbar")) {
        termit_config_get_boolean(&(p_cfg->fill_tabbar), ls, index);
    } else if (!strcmp(name, "hideSingleTab")) {
        termit_config_get_boolean(&(p_cfg->hide_single_tab), ls, index);
    } else if (!strcmp(name, "hideMenubar")) {
        termit_config_get_boolean(&(p_cfg->hide_menubar), ls, index);
    } else if (!strcmp(name, "hideTabbar")) {
        termit_config_get_boolean(&(p_cfg->hide_tabbar), ls, index);
    } else if (!strcmp(name, "showBorder")) {
        termit_config_get_boolean(&(p_cfg->show_border), ls, index);
    } else if (!strcmp(name, "startMaximized")) {
        termit_config_get_boolean(&(p_cfg->start_maximized), ls, index);
    } else if (!strcmp(name, "hideTitlebarWhenMaximized")) {
        termit_config_get_boolean(&(p_cfg->hide_titlebar_when_maximized), ls, index);
    } else if (!strcmp(name, "scrollbackLines")) {
        termit_config_getuint(&(p_cfg->scrollback_lines), ls, index);
    } else if (!strcmp(name, "allowChangingTitle")) {
        termit_config_get_boolean(&(p_cfg->allow_changing_title), ls, index);
    } else if (!strcmp(name, "audibleBell")) {
        termit_config_get_boolean(&(p_cfg->audible_bell), ls, index);
    } else if (!strcmp(name, "scrollOnOutput")) {
        termit_config_get_boolean(&(p_cfg->scroll_on_output), ls, index);
    } else if (!strcmp(name, "scrollOnKeystroke")) {
        termit_config_get_boolean(&(p_cfg->scroll_on_keystroke), ls, index);
    } else if (!strcmp(name, "urgencyOnBell")) {
        termit_config_get_boolean(&(p_cfg->urgency_on_bell), ls, index);
    } else if (!strcmp(name, "getWindowTitle")) {
        termit_config_get_function(&(p_cfg->get_window_title_callback), ls, index);
    } else if (!strcmp(name, "tabPos")) {
        termit_config_get_position(&(p_cfg->tab_pos), ls, index);
    } else if (!strcmp(name, "getTabTitle")) {
        termit_config_get_function(&(p_cfg->get_tab_title_callback), ls, index);
    } else if (!strcmp(name, "setStatusbar")) {
        termit_config_get_function(&(p_cfg->get_statusbar_callback), ls, index);
    } else if (!strcmp(name, "backspaceBinding")) {
        termit_config_get_erase_binding(&(p_cfg->default_bksp), ls, index);
    } else if (!strcmp(name, "deleteBinding")) {
        termit_config_get_erase_binding(&(p_cfg->default_delete), ls, index);
    } else if (!strcmp(name, "cursorBlinkMode")) {
        termit_config_get_cursor_blink_mode(&(p_cfg->default_blink), ls, index);
    } else if (!strcmp(name, "cursorShape")) {
        termit_config_get_cursor_shape(&(p_cfg->default_shape), ls, index);
    } else if (!strcmp(name, "colormap")) {
        termit_lua_load_colormap(ls, index, &configs.style.colors, &configs.style.colors_size);
    } else if (!strcmp(name, "matches")) {
        if (termit_lua_load_table(ls, matchesLoader, index, configs.matches)
                != TERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("failed to load matches");
        }
    } else if (!strcmp(name, "geometry")) {
        gchar* geometry_str = NULL;
        termit_config_get_string(&geometry_str, ls, index);
        if (geometry_str) {
            unsigned int cols = 0, rows = 0;
            int tmp1 = 0, tmp2 = 0;
            XParseGeometry(geometry_str, &tmp1, &tmp2, &cols, &rows);
            if ((cols != 0) && (rows != 0)) {
                p_cfg->cols = cols;
                p_cfg->rows = rows;
            }
        }
        g_free(geometry_str);
    } else if (!strcmp(name, "tabs")) {
        if (lua_istable(ls, index)) {
            if (!configs.default_tabs) {
                configs.default_tabs = g_array_new(FALSE, TRUE, sizeof(struct TabInfo));
            }
            TRACE("tabs at index: %d tabs.size=%d", index, configs.default_tabs->len);
            if (termit_lua_load_table(ls, tabsLoader, index, configs.default_tabs)
                    != TERMIT_LUA_TABLE_LOADER_OK) {
                ERROR("openTab failed");
            }
        } else {
            ERROR("expecting table");
        }
    }
}

static void termit_lua_add_package_path(const gchar* path)
{
    gchar* luaCmd = g_strdup_printf("package.path = package.path .. \";%s/?.lua\"", path);
    int s = luaL_dostring(L, luaCmd);
    termit_lua_report_error(__FILE__, __LINE__, s);
    g_free(luaCmd);
}

static gchar** termit_system_path()
{
    const gchar *configSystem = g_getenv("XDG_CONFIG_DIRS");
    gchar* xdgConfigDirs = NULL;
    if (configSystem) {
        xdgConfigDirs = g_strdup_printf("%s:/etc/xdg", configSystem);
    } else {
        xdgConfigDirs = g_strdup("/etc/xdg");
    }
    gchar** systemPaths = g_strsplit(xdgConfigDirs, ":", 0);
    g_free(xdgConfigDirs);
    return systemPaths;
}

static gchar* termit_user_path()
{
    const gchar *configHome = g_getenv("XDG_CONFIG_HOME");
    if (configHome) {
        return g_strdup(configHome);
    } else {
        return g_strdup_printf("%s/.config", g_getenv("HOME"));
    }
}

static void load_init(const gchar* initFile)
{
    const gchar *configFile = "rc.lua";
    gchar** systemPaths = termit_system_path();
    guint i = 0;
    gchar* systemPath = systemPaths[i];
    while (systemPath) {
        if (g_file_test(systemPath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR) == TRUE) {
            termit_lua_add_package_path(systemPath);
        }
        systemPath = systemPaths[++i];
    }
    gchar* userPath = termit_user_path();
    termit_lua_add_package_path(userPath);

    gchar* fullPath = NULL;
    if (initFile) {
        fullPath = g_strdup(initFile);
    } else {
        fullPath = g_strdup_printf("%s/termit/%s", userPath, configFile);
        if (g_file_test(fullPath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
            TRACE("%s not found", fullPath);
            g_free(fullPath);

            i = 0;
            gchar* systemPath = systemPaths[i];
            while (systemPath) {
                fullPath = g_strdup_printf("%s/termit/%s", systemPath, configFile);
                if (g_file_test(fullPath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
                    TRACE("%s not found", fullPath);
                    g_free(fullPath);
                    fullPath = NULL;
                } else {
                    break;
                }
                systemPath = systemPaths[++i];
            }
        }
    }
    g_strfreev(systemPaths);
    g_free(userPath);
    if (fullPath) {
        TRACE("using config: %s", fullPath);
        int s = luaL_loadfile(L, fullPath);
        termit_lua_report_error(__FILE__, __LINE__, s);
        g_free(fullPath);

        s = lua_pcall(L, 0, LUA_MULTRET, 0);
        termit_lua_report_error(__FILE__, __LINE__, s);
    } else {
        ERROR("config file %s not found", configFile);
    }
}

int termit_lua_fill_tab(int tab_index, lua_State* ls)
{
    TERMIT_GET_TAB_BY_INDEX(pTab, tab_index, return 0);
    lua_newtable(ls);
    TERMIT_TAB_ADD_STRING("title", pTab->title);
    TERMIT_TAB_ADD_STRING("command", pTab->argv[0]);
    TERMIT_TAB_ADD_STRING("argv", "");
    // FIXME: add argv
    TERMIT_TAB_ADD_STRING("encoding", pTab->encoding);
    gchar* working_dir = termit_get_pid_dir(pTab->pid);
    TERMIT_TAB_ADD_STRING("workingDir", working_dir);
    g_free(working_dir);
    TERMIT_TAB_ADD_NUMBER("pid", pTab->pid);
    TERMIT_TAB_ADD_STRING("font", pTab->style.font_name);
    TERMIT_TAB_ADD_NUMBER("fontSize", pango_font_description_get_size(pTab->style.font)/PANGO_SCALE);
    TERMIT_TAB_ADD_STRING("backspaceBinding", termit_erase_binding_to_string(pTab->bksp_binding));
    TERMIT_TAB_ADD_STRING("deleteBinding", termit_erase_binding_to_string(pTab->delete_binding));
    TERMIT_TAB_ADD_STRING("cursorBlinkMode", termit_cursor_blink_mode_to_string(pTab->cursor_blink_mode));
    TERMIT_TAB_ADD_STRING("cursorShape", termit_cursor_shape_to_string(pTab->cursor_shape));
    return 1;
}

static int termit_lua_tabs_index(lua_State* ls)
{
    if (lua_isnumber(ls, 1)) {
        TRACE_MSG("index is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, -1);
    TRACE("tab_index:%d", tab_index);
    return termit_lua_fill_tab(tab_index - 1, ls);
}

static int termit_lua_tabs_newindex(lua_State* ls)
{
    ERROR("'tabs' is read-only variable");
    return 0;
}

static void termit_lua_init_tabs()
{
    lua_newtable(L);
    luaL_newmetatable(L, "tabs_meta");
    lua_pushcfunction(L, termit_lua_tabs_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, termit_lua_tabs_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "tabs");
}

static const gchar* termit_init_file = NULL;

void termit_lua_load_config()
{
    load_init(termit_init_file);
    termit_config_trace();

    trace_menus(configs.user_menus);
    trace_menus(configs.user_popup_menus);
}

void termit_lua_init(const gchar* initFile)
{
    L = luaL_newstate();
    luaL_openlibs(L);

    if (!termit_init_file) {
        termit_init_file = g_strdup(initFile);
    }
    termit_lua_init_tabs();
    termit_lua_init_api();
    termit_keys_set_defaults();
    termit_lua_load_config();
}
