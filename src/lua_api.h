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

#ifndef TERMIT_LUA_API_H
#define TERMIT_LUA_API_H

struct lua_State;

void termit_lua_init_api();
void termit_lua_close();
void termit_lua_report_error(const char* file, int line, int status);
void termit_lua_init(const gchar* initFile);
void termit_lua_load_config();
void termit_lua_execute(const gchar* cmd);
int termit_lua_dofunction(int f);
int termit_lua_dofunction2(int f, const char* arg1);
int termit_lua_domatch(int f, const gchar* matchedText);
void termit_lua_unref(int* lua_callback);
gchar* termit_lua_getTitleCallback(int f, const gchar* title);
gchar* termit_lua_getStatusbarCallback(int f, guint page);
int termit_get_lua_func(const char* name);

typedef enum {TERMIT_LUA_TABLE_LOADER_OK, TERMIT_LUA_TABLE_LOADER_FAILED} TermitLuaTableLoaderResult;
typedef void (*TermitLuaTableLoaderFunc)(const gchar*, struct lua_State*, int, void*);
TermitLuaTableLoaderResult termit_lua_load_table(struct lua_State* ls, TermitLuaTableLoaderFunc func,
        const int tableIndex, void* data);
int termit_lua_fill_tab(int tab_index, struct lua_State* ls);
/**
 * Loaders
 * */
void termit_lua_options_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void termit_lua_keys_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void termit_lua_tab_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void termit_lua_load_colormap(struct lua_State* ls, int index, GdkRGBA** colors, glong* sz);
void termit_config_get_string(gchar** opt, struct lua_State* ls, int index);
void termit_config_get_double(double* opt, struct lua_State* ls, int index);
void termit_config_getuint(guint* opt, struct lua_State* ls, int index);
void termit_config_get_boolean(gboolean* opt, struct lua_State* ls, int index);
void termit_config_get_function(int* opt, struct lua_State* ls, int index);
void termit_config_get_color(GdkRGBA** opt, struct lua_State* ls, int index);
void termit_config_get_erase_binding(VteEraseBinding* opt, struct lua_State* ls, int index);
void termit_config_get_cursor_blink_mode(VteCursorBlinkMode* opt, struct lua_State* ls, int index);
void termit_config_get_cursor_shape(VteCursorShape* opt, struct lua_State* ls, int index);

#define TERMIT_TAB_ADD_NUMBER(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushnumber(ls, value); \
    lua_rawset(ls, -3); \
}
#define TERMIT_TAB_ADD_STRING(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushstring(ls, value); \
    lua_rawset(ls, -3); \
}
#define TERMIT_TAB_ADD_VOID(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushlightuserdata(ls, value); \
    lua_rawset(ls, -3); \
}
#define TERMIT_TAB_ADD_BOOLEAN(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushboolean(ls, value); \
    lua_rawset(ls, -3); \
}
#define TERMIT_TAB_ADD_CALLBACK(name, value) {\
    lua_pushstring(ls, name); \
    lua_rawgeti(ls, LUA_REGISTRYINDEX, value); \
    lua_rawset(ls, -3); \
}
#endif /* TERMIT_LUA_API_H */
