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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "keybindings.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"

extern lua_State* L;

TermitLuaTableLoaderResult termit_lua_load_table(lua_State* ls, TermitLuaTableLoaderFunc func, void* data)
{
    if (!data) {
        TRACE_MSG("data is NULL: skipping");
        return TERMIT_LUA_TABLE_LOADER_FAILED;
    }
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("tabInfo not defined: skipping");
        return TERMIT_LUA_TABLE_LOADER_FAILED;
    }
    if (!lua_istable(ls, 1)) {
        TRACE_MSG("tabInfo is not table: skipping");
        lua_pop(ls, 1);
        return TERMIT_LUA_TABLE_LOADER_FAILED;
    }
    lua_pushnil(ls);
    while (lua_next(ls, 1) != 0) {
        if (lua_isstring(ls, -2)) {
            const gchar* name = lua_tostring(ls, -2);
            func(name, ls, -1, data);
        }
        lua_pop(ls, 1);
    }
    lua_pop(ls, 1);
    return TERMIT_LUA_TABLE_LOADER_OK;
}


void termit_lua_execute(const gchar* cmd)
{
    TRACE("executing script: %s", cmd);
    int s = luaL_dostring(L, cmd);
    termit_lua_report_error(__FILE__, __LINE__, s);
}

void termit_lua_report_error(const char* file, int line, int status)
{
    if (status == 0)
        return;
    TRACE_MSG("lua error:");
    g_fprintf(stderr, "%s:%d %s\n", file, line, lua_tostring(L, -1));
    lua_pop(L, 1);
}

static int termit_lua_setOptions(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_lua_load_table(ls, termit_lua_options_loader, &configs);
    return 0;
}

int termit_lua_dofunction(int f)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        if (lua_pcall(ls, 0, 0, 0)) {
            TRACE("error running function (%d): %s", f, lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return 0;
        }
        return 1;
    }
    return 0;
}

void termit_lua_unref(int* lua_callback)
{
    if (*lua_callback) {
        luaL_unref(L, LUA_REGISTRYINDEX, *lua_callback);
        *lua_callback = 0;
    }
}

gchar* termit_lua_getTitleCallback(int f, const gchar* title)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        lua_pushstring(ls, title);
        if (lua_pcall(ls, 1, 1, 0)) {
            TRACE("error running function: %s", lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return NULL;
        }
        if (lua_isstring(ls, -1))
            return g_strdup(lua_tostring(ls, -1));
    }
    return NULL;
}

static int termit_lua_bindKey(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("nil args: skipping");
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("bad args: skipping");
    } else if (lua_isnil(ls, 2)) {
        const char* keybinding = lua_tostring(ls, 1);
        termit_keys_unbind(keybinding);
        TRACE("unbindKey: %s", keybinding);
    } else if (lua_isfunction(ls, 2)) {
        const char* keybinding = lua_tostring(ls, 1);
        int func = luaL_ref(ls, LUA_REGISTRYINDEX);
        termit_keys_bind(keybinding, func);
        TRACE("bindKey: %s - %d", keybinding, func);
    }
    return 0;
}

static int termit_lua_bindMouse(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("nil args: skipping");
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("bad args: skipping");
    } else if (lua_isnil(ls, 2)) {
        const char* mousebinding = lua_tostring(ls, 1);
        termit_mouse_unbind(mousebinding);
        TRACE("unbindMouse: %s", mousebinding);
    } else if (lua_isfunction(ls, 2)) {
        const char* mousebinding = lua_tostring(ls, 1);
        int func = luaL_ref(ls, LUA_REGISTRYINDEX);
        termit_mouse_bind(mousebinding, func);
        TRACE("bindMouse: %s - %d", mousebinding, func);
    }
    return 0;
}

static int termit_lua_toggleMenubar(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_toggle_menubar();
    return 0;
}

static void tabLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct TabInfo* ti = (struct TabInfo*)data;
    if (!strcmp(name, "title") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->name = g_strdup(value);
    } else if (!strcmp(name, "command") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->command = g_strdup(value);
    } else if (!strcmp(name, "encoding") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->encoding = g_strdup(value);
    } else if (!strcmp(name, "workingDir") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->working_dir = g_strdup(value);
    }
}

static int termit_lua_nextTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_next_tab();
    return 0;
}

static int termit_lua_prevTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_prev_tab();
    return 0;
}

static int termit_lua_copy(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_copy();
    return 0;
}

static int termit_lua_paste(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_paste();
    return 0;
}

static int termit_lua_openTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_istable(ls, 1)) {
        struct TabInfo ti = {0};
        if (termit_lua_load_table(ls, tabLoader, &ti) != TERMIT_LUA_TABLE_LOADER_OK)
            return 0;
        termit_append_tab_with_details(&ti);
        g_free(ti.name);
        g_free(ti.command);
        g_free(ti.encoding);
        g_free(ti.working_dir);
    } else {
        termit_append_tab();
    }
    return 0;
}

static int termit_lua_closeTab(lua_State* ls)
{
    termit_close_tab();
    TRACE_FUNC;
    return 0;
}

static void termit_load_colormap(lua_State* ls, GdkColormap* colormap)
{
    int size = lua_objlen(ls, 1);
    if ((size != 8) && (size != 16) && (size != 24)) {
        ERROR("bad colormap length: %d", size);
        return;
    }
    colormap->size = size;
    colormap->colors = g_malloc0(colormap->size * sizeof(GdkColor));
    int i = 0;
    lua_pushnil(ls);  /* first key */
    while (lua_next(ls, 1) != 0) {
        /* uses 'key' (at index -2) and 'value' (at index -1) */
        const int valueIndex = -1;
//        TRACE("%s - %s", lua_typename(ls, lua_type(ls, -2)), lua_typename(ls, lua_type(ls, valueIndex)));
        if (!lua_isnil(ls, valueIndex) && lua_isstring(ls, valueIndex)) {
            const gchar* colorStr = lua_tostring(ls, valueIndex);
//            TRACE("%d - %s", i, colorStr);
            if (!gdk_color_parse(colorStr, &colormap->colors[i])) {
                ERROR("failed to parse color: %d - %s", i, colorStr);
            }
        }
        /* removes 'value'; keeps 'key' for next iteration */
        lua_pop(ls, 1);
        i++;
    }
}

static int termit_lua_setColormap(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    
    if (configs.style.colormap) {
        g_free(configs.style.colormap->colors);
    } else {
        configs.style.colormap = g_malloc0(sizeof(GdkColormap));
    }
    termit_load_colormap(ls, configs.style.colormap);
    return 0;
}

static int termit_lua_setMatches(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    termit_lua_load_table(ls, termit_lua_matches_loader, configs.matches);
    return 0;
}

static int termit_lua_setKbPolicy(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no kbPolicy defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("kbPolicy is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    TRACE("setKbPolicy: %s", val);
    if (!strcmp(val, "keycode"))
        termit_set_kb_policy(TermitKbUseKeycode);
    else if (!strcmp(val, "keysym"))
        termit_set_kb_policy(TermitKbUseKeysym);
    else {
        ERROR("unknown kbPolicy: %s", val);
    }
    return 0;
}

static int loadMenu(lua_State* ls, GArray* menus)
{
    int res = 0;
    if (lua_isnil(ls, 1) || lua_isnil(ls, 2)) {
        TRACE_MSG("menu not defined: skipping");
        res = -1;
    } else if (!lua_istable(ls, 1) || !lua_isstring(ls, 2)) {
        TRACE_MSG("menu is not table: skipping");
        res = -1;
    } else {
        const gchar* menuName = lua_tostring(ls, 2);
        TRACE("Menu: %s", menuName);
        lua_pushnil(ls);
        struct UserMenu um;
        um.name = g_strdup(menuName);
        um.items = g_array_new(FALSE, TRUE, sizeof(struct UserMenuItem));
        while (lua_next(ls, 1) != 0) {
            if (lua_isstring(ls, -2)) {
                struct UserMenuItem umi = {0};
                lua_pushnil(ls);
                while (lua_next(ls, -2) != 0) {
                    if (lua_isstring(ls, -2)) {
                        const gchar* name = lua_tostring(ls, -2);
                        if (!strcmp(name, "name")) {
                            const gchar* value = lua_tostring(ls, -1);
                            umi.name = g_strdup(value);
                        } else if (!strcmp(name, "action")) {
                            if (lua_isfunction(ls, -1)) {
                                umi.lua_callback = luaL_ref(ls, LUA_REGISTRYINDEX);
                                lua_pushinteger(ls, 0);
                            }
                        } else if (!strcmp(name, "accel")) {
                            const gchar* value = lua_tostring(ls, -1);
                            umi.accel = g_strdup(value);
                        }
                    }
                    lua_pop(ls, 1);
                }
                g_array_append_val(um.items, umi);
            }
            lua_pop(ls, 1);
        }
        g_array_append_val(menus, um);
    }
    lua_pop(ls, 1);

    return res;
}

static int termit_lua_addMenu(lua_State* ls)
{
    if (loadMenu(ls, configs.user_menus) < 0) {
        ERROR("addMenu failed");
    }
    return 0;
}

static int termit_lua_addPopupMenu(lua_State* ls)
{
    if (loadMenu(ls, configs.user_popup_menus) < 0) {
        ERROR("addMenu failed");
    }
    return 0;
}

static int termit_lua_setEncoding(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no encoding defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("encoding is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    termit_set_encoding(val);
    return 0;
}

static int termit_lua_activateTab(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabNum defined: skipping");
        return 0;
    } else if (!lua_isnumber(ls, 1)) {
        TRACE_MSG("tabNum is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, 1);
    termit_activate_tab(tab_index - 1);
    return 0;
}

static int termit_lua_currentTab(lua_State* ls)
{
    return termit_lua_fill_tab(termit_get_current_tab_index(), ls);
}

static int termit_lua_currentTabIndex(lua_State* ls)
{
    lua_pushinteger(ls, termit_get_current_tab_index() + 1);
    return 1;
}

static int termit_lua_changeTab(lua_State* ls)
{
    return 0;
}

static int termit_lua_loadSessionDialog(lua_State* ls)
{
    termit_on_load_session();
    return 0;
}

static int termit_lua_saveSessionDialog(lua_State* ls)
{
    termit_on_save_session();
    return 0;
}

static int termit_lua_preferencesDialog(lua_State* ls)
{
    termit_on_edit_preferences();
    return 0;
}

static int termit_lua_setTabTitleDialog(lua_State* ls)
{
    termit_on_set_tab_name();
    return 0;
}

static int termit_lua_setTabTitle(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabName defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("tabName is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX2(pTab, page, 0);
    termit_tab_set_title(pTab, val);
    pTab->custom_tab_name = TRUE;
    return 0;
}

static int termit_lua_setWindowTitle(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no title defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("title is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    termit_set_window_title(val);
    return 0;
}

static int termit_lua_setTabColor__(lua_State* ls, void (*callback)(gint, const GdkColor*))
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no color defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("color is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    GdkColor color;
    if (gdk_color_parse(val, &color) == TRUE)
        callback(-1, &color);
    return 0;
}

static int termit_lua_setTabForegroundColor(lua_State* ls)
{
    return termit_lua_setTabColor__(ls, &termit_tab_set_color_foreground_by_index);
}

static int termit_lua_setTabBackgroundColor(lua_State* ls)
{
    return termit_lua_setTabColor__(ls, &termit_tab_set_color_background_by_index);
}

static int termit_lua_setTabFont(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no font defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("font is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    termit_tab_set_font_by_index(-1, val);
    return 0;
}

static int termit_lua_spawn(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no command defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("command is not string: skipping");
        return 0;
    }
    GError *err = NULL;
    const gchar* val =  lua_tostring(ls, 1);
    g_spawn_command_line_async(val, &err);
    return 0;
}

static int termit_lua_selection(lua_State* ls)
{
    gchar* buff = termit_get_selection();
    if (!buff)
        return 0;
    TRACE("buff=%s", buff);
    lua_pushstring(ls, buff);
    g_free(buff);
    return 1;
}

static int termit_lua_reconfigure(lua_State* ls)
{
    termit_reconfigure();
    termit_config_trace();
    return 0;
}

static int termit_lua_quit(lua_State* ls)
{
    termit_on_exit();
    return 0;
}

static int termit_submenu__index(lua_State* ls)
{
    size_t len = 0;
    const char *buf = luaL_checklstring(ls, 2, &len);
    if (!buf) {
        ERROR("invalid argument");
        lua_pushnil(ls);
        return 1;
    }
    lua_pushnil(ls); // may be it would be better to use stack instead of tab field
    TRACE("searching [%s]", buf);
    GtkWidget* submenu = NULL;
    while (lua_next(ls, 1) != 0) {
        if (lua_isstring(ls, -2) && lua_islightuserdata(ls, -1)) {
            const gchar* name = lua_tostring(ls, -2);
            if (strcmp(name, "submenu") != 0) {
                continue;                
            }
            submenu = lua_touserdata(ls, -1);
        }
        lua_pop(ls, 1);
    }
    lua_pop(ls, 1);
    if (!submenu) {
        ERROR("usermenu not found: %s", buf);
        lua_pushnil(ls);
        return 1;
    }
    GList* items = gtk_container_get_children(GTK_CONTAINER(submenu));
    while (items) {
        GtkMenuItem* mi = GTK_MENU_ITEM(items->data);
        items = items->next;
        if (strlen(gtk_menu_item_get_label(mi)) == 0) {
            continue; // skip separators
        }
        TRACE("submenu.item [%s]", gtk_menu_item_get_label(mi));
        if (strcmp(buf, gtk_menu_item_get_label(mi)) == 0) {
            struct UserMenuItem* umi = (struct UserMenuItem*)g_object_get_data(G_OBJECT(mi),
                    TERMIT_USER_MENU_ITEM_DATA);
            if (!umi) {
                ERROR("usermenu not found: %s", buf);
                lua_pushnil(ls);
                return 1;
            }
            TRACE("  item: name=%5.5s accel=%5.5s callback=%d", umi->name, umi->accel, umi->lua_callback);
            lua_newtable(ls);
            TERMIT_TAB_ADD_STRING("name", umi->name);
            TERMIT_TAB_ADD_STRING("accel", umi->accel);
            TERMIT_TAB_ADD_CALLBACK("action", umi->lua_callback);
            return 1;
        }
    }
    lua_pushnil(ls);
    return 1;
}

static int termit_menu__index(lua_State* ls)
{
    size_t len = 0;
    const char *buf = luaL_checklstring(ls, 2, &len);
    if (!buf) {
        ERROR("invalid argument");
        lua_pushnil(ls);
        return 1;
    }
    TRACE("args=%s len=%d", buf, len);
    GList* menus = gtk_container_get_children(GTK_CONTAINER(termit.menu_bar));
    while (menus) {
        GtkMenuItem* mi = GTK_MENU_ITEM(menus->data);
        if (mi && strcmp(gtk_menu_item_get_label(mi), buf) == 0) {
            TRACE("found menu [%s]", buf);
            lua_newtable(ls);
            TERMIT_TAB_ADD_VOID("submenu", gtk_menu_item_get_submenu(mi));
            luaL_newmetatable(ls, "termit_submenu_meta");
            lua_pushcfunction(ls, termit_submenu__index);
            lua_setfield(ls, -2, "__index");
            lua_setmetatable(ls, -2);
            return 1;
        }
        menus = menus->next;
    }
    lua_pushnil(ls);
    return 1;
}

static void termit_prepare_menu()
{
    lua_newtable(L);
    luaL_newmetatable(L, "termit_menu_meta");
    lua_pushcfunction(L, termit_menu__index);
    lua_setfield(L, -2, "__index");
    /*lua_pushcfunction(L, termit_menu__newindex);*/
    /*lua_setfield(L, -2, "__newindex");*/
    lua_setmetatable(L, -2);
    lua_setfield(L, LUA_GLOBALSINDEX, "termitMenu");
}

struct TermitLuaFunction
{
    const char* lua_func_name;
    lua_CFunction c_func;
    int lua_func;
} functions[] = {
    {"setOptions", termit_lua_setOptions, 0},
    {"bindKey", termit_lua_bindKey, 0},
    {"bindMouse", termit_lua_bindMouse, 0},
    {"setKbPolicy", termit_lua_setKbPolicy, 0},
    {"setMatches", termit_lua_setMatches, 0},
    {"setColormap", termit_lua_setColormap, 0},
    {"openTab", termit_lua_openTab, 0},
    {"nextTab", termit_lua_nextTab, 0},
    {"prevTab", termit_lua_prevTab, 0},
    {"activateTab", termit_lua_activateTab, 0},
    {"changeTab", termit_lua_changeTab, 0},
    {"closeTab", termit_lua_closeTab, 0},
    {"copy", termit_lua_copy, 0},
    {"paste", termit_lua_paste, 0},
    {"addMenu", termit_lua_addMenu, 0},
    {"addPopupMenu", termit_lua_addPopupMenu, 0},
    {"currentTabIndex", termit_lua_currentTabIndex, 0},
    {"currentTab", termit_lua_currentTab, 0},
    {"setEncoding", termit_lua_setEncoding, 0},
    {"setTabTitle", termit_lua_setTabTitle, 0},
    {"setWindowTitle", termit_lua_setWindowTitle, 0},
    {"setTabForegroundColor", termit_lua_setTabForegroundColor, 0},
    {"setTabBackgroundColor", termit_lua_setTabBackgroundColor, 0},
    {"setTabFont", termit_lua_setTabFont, 0},
    {"toggleMenu", termit_lua_toggleMenubar, 0},
    {"reconfigure", termit_lua_reconfigure, 0},
    {"spawn", termit_lua_spawn, 0},
    {"selection", termit_lua_selection, 0},
    {"setTabTitleDlg", termit_lua_setTabTitleDialog, 0},
    {"loadSessionDlg", termit_lua_loadSessionDialog, 0},
    {"saveSessionDlg", termit_lua_saveSessionDialog, 0},
    {"preferencesDlg", termit_lua_preferencesDialog, 0},
    {"quit", termit_lua_quit, 0}
};

int termit_get_lua_func(const char* name)
{
    int i = 0;
    for (; i < sizeof(functions)/sizeof(functions[0]); ++i) {
        if (strcmp(name, functions[i].lua_func_name) == 0) {
            return functions[i].lua_func;
        }
    }
    ERROR("not found lua function by name [%s]", name);
    return 0;
}


void termit_lua_init_api()
{
    int i = 0;
    for (; i < sizeof(functions)/sizeof(functions[0]); ++i) {
        lua_pushcfunction(L, functions[i].c_func);
        functions[i].lua_func = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_rawgeti(L, LUA_REGISTRYINDEX, functions[i].lua_func);
        lua_setglobal(L, functions[i].lua_func_name);
        TRACE("%s [%d]", functions[i].lua_func_name, functions[i].lua_func);
    }
    termit_prepare_menu();
}

