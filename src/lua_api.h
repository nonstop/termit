#ifndef TERMIT_LUA_API_H
#define TERMIT_LUA_API_H

struct lua_State;

void termit_init_lua_api();
void termit_lua_close();
void termit_report_lua_error(const char* file, int line, int status);
void termit_lua_init(const gchar* initFile);
void termit_load_lua_config();
void termit_lua_execute(const gchar* cmd);
int termit_lua_dofunction(int f);
void termit_lua_unref(int* lua_callback);
gchar* termit_lua_getTitleCallback(int f, const gchar* title);
/**
 * Loaders
 * */
void termit_options_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void termit_kb_loader(const gchar* name, struct lua_State* ls, int index, void* data);

#endif /* TERMIT_LUA_API_H */

