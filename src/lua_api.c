#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "configs.h"
#include "lua_api.h"

extern lua_State* L;

typedef enum {LOADER_OK, LOADER_FAILED} LoaderResult;
typedef void (*LoaderFunc)(const gchar*, lua_State*, int, void*);
static LoaderResult load_lua_table(lua_State* ls, LoaderFunc func, void* data)
{
    if (!data)
    {
        TRACE_MSG("data is NULL: skipping");
        return LOADER_FAILED;
    }
    if (lua_isnil(ls, 1))
    {
        TRACE_MSG("tabInfo not defined: skipping");
        return LOADER_FAILED;
    }
    if (!lua_istable(ls, 1))
    {
        TRACE_MSG("tabInfo is not table: skipping");
        lua_pop(ls, 1);
        return LOADER_FAILED;
    }
    lua_pushnil(ls);
    while (lua_next(ls, 1) != 0)
    {
        if (lua_isstring(ls, -2))
        {
            const gchar* name = lua_tostring(ls, -2);
            func(name, ls, -1, data);
        }
        lua_pop(ls, 1);
    }
    lua_pop(ls, 1);
    return LOADER_OK;
}


void termit_lua_execute(const gchar* cmd)
{
    TRACE("executing script: %s", cmd);
    int s = luaL_dostring(L, cmd);
    termit_report_lua_error(s);
}

void termit_report_lua_error(int status)
{
    if (status == 0)
        return;
    TRACE_MSG("lua error:");
    g_fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
}

static int termit_lua_setOptions(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    load_lua_table(ls, termit_options_loader, &configs);
    trace_configs();
    return 0;
}

static int termit_lua_setKeys(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    load_lua_table(ls, termit_kb_loader, configs.key_bindings);
    trace_keybindings();
    return 0;
}

static void tabLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct TabInfo* ti = (struct TabInfo*)data;
    if (!strcmp(name, "name") && lua_isstring(ls, index))
    {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->name = g_strdup(value);
    }
    else if (!strcmp(name, "command") && lua_isstring(ls, index))
    {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->command = g_strdup(value);
    }
    else if (!strcmp(name, "encoding") && lua_isstring(ls, index))
    {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->encoding = g_strdup(value);
    }
    else if (!strcmp(name, "working_dir") && lua_isstring(ls, index))
    {
        const gchar* value = lua_tostring(ls, index);
        TRACE("  %s - %s", name, value);
        ti->working_dir = g_strdup(value);
    }
}

static int termit_lua_openTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    struct TabInfo ti = {0};
    if (load_lua_table(ls, tabLoader, &ti) != LOADER_OK)
        return 0;
    termit_append_tab_with_details(&ti);
    g_free(ti.name);
    g_free(ti.command);
    g_free(ti.encoding);
    g_free(ti.working_dir);
    return 0;
}

static int termit_lua_closeTab(lua_State* ls)
{
    termit_close_tab();
    TRACE_FUNC;
    return 0;
}

static int termit_lua_setKbPolicy(lua_State* ls)
{
    if (lua_isnil(ls, 1))
    {
        TRACE_MSG("no kbPolicy defined: skipping");
        return 0;
    }
    else if (!lua_isstring(ls, 1))
    {
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
    if (lua_isnil(ls, 1) || lua_isnil(ls, 2))
    {
        TRACE_MSG("menu not defined: skipping");
        res = -1;
    }
    else if (!lua_istable(ls, 1) || !lua_isstring(ls, 2))
    {
        TRACE_MSG("menu is not table: skipping");
        res = -1;
    }
    else
    {
        const gchar* menuName = lua_tostring(ls, 2);
        TRACE("Menu: %s", menuName);
        lua_pushnil(ls);
        struct UserMenu um;
        um.name = g_strdup(menuName);
        um.items = g_array_new(FALSE, TRUE, sizeof(struct UserMenuItem));
        while (lua_next(ls, 1) != 0)
        {
            if (lua_isstring(ls, -2))
            {
                struct UserMenuItem umi = {0};
                lua_pushnil(ls);
                while (lua_next(ls, -2) != 0)
                {
                    if (lua_isstring(ls, -2))
                    {
                        const gchar* name = lua_tostring(ls, -2);
                        const gchar* value = lua_tostring(ls, -1);
                        if (!strcmp(name, "name"))
                            umi.name = g_strdup(value);
                        else if (!strcmp(name, "action"))
                            umi.userFunc = g_strdup(value);
                    }
                    lua_pop(ls, 1);
                }
                g_array_append_val(um.items, umi);
            }
            lua_pop(ls, 1);
        }
        g_array_append_val(menus, um);
    }
    TRACE_MSG(__FUNCTION__);
    lua_pop(ls, 1);
    TRACE_MSG(__FUNCTION__);

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
    if (lua_isnil(ls, 1))
    {
        TRACE_MSG("no encoding defined: skipping");
        return 0;
    }
    else if (!lua_isstring(ls, 1))
    {
        TRACE_MSG("encoding is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    termit_set_encoding(val);
    return 0;
}

static int termit_lua_setTabName(lua_State* ls)
{
    if (lua_isnil(ls, 1))
    {
        TRACE_MSG("no tabName defined: skipping");
        return 0;
    }
    else if (!lua_isstring(ls, 1))
    {
        TRACE_MSG("tabName is not string: skipping");
        return 0;
    }
    const gchar* val =  lua_tostring(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    termit_set_tab_name(page, val);
    return 0;
}
static int termit_lua_reconfigure(lua_State* ls)
{
    termit_reconfigure();
    return 0;
}

void termit_init_lua_api()
{
    TRACE_FUNC;
    lua_register(L, "setOptions", termit_lua_setOptions);
    lua_register(L, "setKeys", termit_lua_setKeys);
    lua_register(L, "setKbPolicy", termit_lua_setKbPolicy);
    lua_register(L, "openTab", termit_lua_openTab);
    lua_register(L, "closeTab", termit_lua_closeTab);
    lua_register(L, "addMenu", termit_lua_addMenu);
    lua_register(L, "addPopupMenu", termit_lua_addPopupMenu);
    lua_register(L, "setEncoding", termit_lua_setEncoding);
    lua_register(L, "setTabName", termit_lua_setTabName);
    lua_register(L, "reconfigure", termit_lua_reconfigure);
}

