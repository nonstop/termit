/*  Copyright (C) 2007-2010, Evgeny Ratnikov

    This file is part of termit.
    termit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 
    as published by the Free Software Foundation.
    termit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with termit. If not, see <http://www.gnu.org/licenses/>.*/

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
    gint i = 0;
    for (; i<menus->len; ++i) {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        TRACE("%s items %d", um->name, um->items->len);
        gint j = 0;
        for (; j<um->items->len; ++j) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, j);
            TRACE("  %s: (%d) [%s]", umi->name, umi->lua_callback, umi->accel);
        }
    }
#endif
}

static void config_getstring(gchar** opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        if (*opt)
            g_free(*opt);
        *opt = g_strdup(lua_tostring(ls, index));
    }
}
static void config_getdouble(double* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isnumber(ls, index))
        *opt = lua_tonumber(ls, index);
}

static void config_getuint(guint* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isnumber(ls, index))
        *opt = lua_tointeger(ls, index);
}
static void config_getboolean(gboolean* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isboolean(ls, index))
        *opt = lua_toboolean(ls, index);
}
static void config_getfunction(int* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isfunction(ls, index)) {
        *opt = luaL_ref(ls, LUA_REGISTRYINDEX); // luaL_ref pops value so we restore stack size
        lua_pushnil(ls);
    }
}
static void config_getcolor(GdkColor* opt, lua_State* ls, int index)
{
    gchar* color_str = NULL;
    config_getstring(&color_str, ls, index);
    TRACE("color_str=%s", color_str);
    if (color_str) {
        GdkColor color = {};
        if (gdk_color_parse(color_str, &color) == TRUE) {
            *opt = color;
        }
    }
    g_free(color_str);
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
    match.regex = g_regex_new(pattern, 0, 0, &err);
    if (err) {
        TRACE("failed to compile regex [%s]: skipping", pattern);
        return;
    }
    match.flags = 0;
    match.pattern = g_strdup(pattern);
    config_getfunction(&match.lua_callback, ls, index);
    g_array_append_val(matches, match);
}

struct ColormapHelper
{
    GdkColormap* cm;
    int idx;
};

static void colormapLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct ColormapHelper* ch = (struct ColormapHelper*)data;
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        const gchar* colorStr = lua_tostring(ls, index);
        if (!gdk_color_parse(colorStr, &(ch->cm->colors[ch->idx]))) {
            ERROR("failed to parse color: %d - %s", ch->idx, colorStr);
        }
    } else {
        ERROR("invalid type in colormap: skipping");
    }
    ++ch->idx;
}

static void tabsLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    if (lua_istable(ls, index)) {
        struct TabInfo ti = {};
        if (termit_lua_load_table(ls, termit_lua_tab_loader, index, &ti)
                != TERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("failed to load tab: %s", lua_tostring(ls, 3));
        }
        termit_append_tab_with_details(&ti);
        g_free(ti.name);
        g_free(ti.command);
        g_free(ti.encoding);
        g_free(ti.working_dir);
    } else {
        ERROR("unknown type instead if tab table: skipping");
        lua_pop(ls, 1);
    }
}

void termit_lua_options_loader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct Configs* p_cfg = (struct Configs*)data;
    if (!strcmp(name, "tabName"))
        config_getstring(&(p_cfg->default_tab_name), ls, index);
    else if (!strcmp(name, "windowTitle"))
        config_getstring(&(p_cfg->default_window_title), ls, index);
    else if (!strcmp(name, "encoding"))
        config_getstring(&(p_cfg->default_encoding), ls, index);
    else if (!strcmp(name, "wordChars"))
        config_getstring(&(p_cfg->default_word_chars), ls, index);
    else if (!strcmp(name, "font"))
        config_getstring(&(p_cfg->style.font_name), ls, index);
    else if (!strcmp(name, "foregroundColor")) 
        config_getcolor(&(p_cfg->style.foreground_color), ls, index);
    else if (!strcmp(name, "backgroundColor")) 
        config_getcolor(&(p_cfg->style.background_color), ls, index);
    else if (!strcmp(name, "showScrollbar"))
        config_getboolean(&(p_cfg->show_scrollbar), ls, index);
    else if (!strcmp(name, "transparency"))
        config_getdouble(&(p_cfg->style.transparency), ls, index);
    else if (!strcmp(name, "fillTabbar"))
        config_getboolean(&(p_cfg->fill_tabbar), ls, index);
    else if (!strcmp(name, "hideSingleTab"))
        config_getboolean(&(p_cfg->hide_single_tab), ls, index);
    else if (!strcmp(name, "hideMenubar"))
        config_getboolean(&(p_cfg->hide_menubar), ls, index);
    else if (!strcmp(name, "scrollbackLines"))
        config_getuint(&(p_cfg->scrollback_lines), ls, index);
    else if (!strcmp(name, "allowChangingTitle"))
        config_getboolean(&(p_cfg->allow_changing_title), ls, index);
    else if (!strcmp(name, "audibleBell"))
        config_getboolean(&(p_cfg->audible_bell), ls, index);
    else if (!strcmp(name, "visibleBell"))
        config_getboolean(&(p_cfg->visible_bell), ls, index);
    else if (!strcmp(name, "urgencyOnBell"))
        config_getboolean(&(p_cfg->urgency_on_bell), ls, index);
    else if (!strcmp(name, "getWindowTitle"))
        config_getfunction(&(p_cfg->get_window_title_callback), ls, index);
    else if (!strcmp(name, "getTabTitle"))
        config_getfunction(&(p_cfg->get_tab_title_callback), ls, index);
    else if (!strcmp(name, "colormap")) {
        if (!lua_isnil(ls, index) && lua_istable(ls, index)) {
            const int size = lua_objlen(ls, index);
            if ((size != 8) && (size != 16) && (size != 24)) {
                ERROR("bad colormap length: %d", size);
                return;
            }
            struct ColormapHelper ch = {g_malloc0(sizeof(GdkColormap)), 0};
            ch.cm->size = size;
            ch.cm->colors = g_malloc0(ch.cm->size * sizeof(GdkColor));
            if (termit_lua_load_table(ls, colormapLoader, index, &ch)
                    == TERMIT_LUA_TABLE_LOADER_OK) {
                if (configs.style.colormap) {
                    g_free(configs.style.colormap->colors);
                }
                configs.style.colormap = ch.cm;
            } else {
                ERROR("failed to load colormap");
            }
        } else {
            ERROR("invalid type in colormap");
        }
    } else if (!strcmp(name, "matches")) {
        if (termit_lua_load_table(ls, matchesLoader, index, configs.matches)
                != TERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("failed to load matches");
        }
    } else if (!strcmp(name, "geometry")) {
        gchar* geometry_str = NULL;
        config_getstring(&geometry_str, ls, index);
        if (geometry_str) {
            uint cols, rows;
            int tmp1, tmp2;
            XParseGeometry(geometry_str, &tmp1, &tmp2, &cols, &rows);
            if ((cols != 0) && (rows != 0)) {
                p_cfg->cols = cols;
                p_cfg->rows = rows;
            }
        }
        g_free(geometry_str);
    } else if (!strcmp(name, "tabs")) {
        if (lua_istable(ls, index)) {
            TRACE("tabs at index: %d", index);
            if (termit_lua_load_table(ls, tabsLoader, index, NULL)
                    != TERMIT_LUA_TABLE_LOADER_OK) {
                ERROR("openTab failed");
            }
        } else {
            ERROR("expecting table");
        }
    }
}

static void load_init(const gchar* initFile)
{
    gchar* fullPath = NULL;
    if (initFile) {
        fullPath = g_strdup(initFile);
    } else {
        const gchar *configHome = g_getenv("XDG_CONFIG_HOME");
        gchar* path = NULL;
        if (configHome)
            path = g_strdup_printf("%s/termit", configHome);
        else
            path = g_strdup_printf("%s/.config/termit", g_getenv("HOME"));
        fullPath = g_strdup_printf("%s/rc.lua", path);
        g_free(path);
    }
    TRACE("config: %s", fullPath);
    int s = luaL_loadfile(L, fullPath);
    termit_lua_report_error(__FILE__, __LINE__, s);
    g_free(fullPath);

    s = lua_pcall(L, 0, LUA_MULTRET, 0);
    termit_lua_report_error(__FILE__, __LINE__, s);
}

int termit_lua_fill_tab(int tab_index, lua_State* ls)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX2(pTab, page, 0);
    lua_newtable(ls);
    TERMIT_TAB_ADD_STRING("title", pTab->title);
    TERMIT_TAB_ADD_STRING("command", pTab->command);
    TERMIT_TAB_ADD_STRING("encoding", pTab->encoding);
    gchar* working_dir = termit_get_pid_dir(pTab->pid);
    TERMIT_TAB_ADD_STRING("workingDir", working_dir);
    g_free(working_dir);
    TERMIT_TAB_ADD_NUMBER("pid", pTab->pid);
    TERMIT_TAB_ADD_STRING("font", pTab->style.font_name);
    TERMIT_TAB_ADD_NUMBER("fontSize", pango_font_description_get_size(pTab->style.font)/PANGO_SCALE);
    return 1;
}

static int termit_lua_tabs_index(lua_State* ls)
{
    if (lua_isnumber(ls, 1)) {
        TRACE_MSG("index is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, 1);
    TRACE("tab_index:%d", tab_index);
    return termit_lua_fill_tab(tab_index, ls);
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
    lua_setfield(L, LUA_GLOBALSINDEX, "tabs");
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

    if (!termit_init_file)
        termit_init_file = g_strdup(initFile);
    termit_lua_init_tabs();
    termit_lua_init_api();
    termit_keys_set_defaults();
    termit_lua_load_config();
}

