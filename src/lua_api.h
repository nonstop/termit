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
void termit_lua_unref(int* lua_callback);
gchar* termit_lua_getTitleCallback(int f, const gchar* title);

typedef enum {TERMIT_LUA_TABLE_LOADER_OK, TERMIT_LUA_TABLE_LOADER_FAILED} TermitLuaTableLoaderResult;
typedef void (*TermitLuaTableLoaderFunc)(const gchar*, struct lua_State*, int, void*);
TermitLuaTableLoaderResult termit_lua_load_table(struct lua_State* ls, TermitLuaTableLoaderFunc func, void* data);
/**
 * Loaders
 * */
void termit_lua_options_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void termit_lua_keys_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void termit_lua_matches_loader(const gchar* name, struct lua_State* ls, int index, void* data);

#endif /* TERMIT_LUA_API_H */

