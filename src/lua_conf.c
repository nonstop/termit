#include <string.h>
#include <X11/Xlib.h> // XParseGeometry

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
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
    for (; i<menus->len; ++i)
    {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        TRACE("%s items %d", um->name, um->items->len);
        gint j = 0;
        for (; j<um->items->len; ++j)
        {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, j);
            TRACE("  %s: %s", umi->name, umi->userFunc);
        }
    }
#endif
}

static void config_getstring(gchar** opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isstring(ls, index))
    {
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

void termit_options_loader(const gchar* name, lua_State* ls, int index, void* data)
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
        config_getstring(&(p_cfg->default_font), ls, index);
    else if (!strcmp(name, "foreground_color")) {
        gchar* color_str = NULL;
        config_getstring(&color_str, ls, index);
        TRACE("color_str=%s", color_str);
        if (color_str) {
            //struct GdkColor color;
            GdkColor color;
            if (gdk_color_parse(color_str, &color) == TRUE) {
                configs.default_foreground_color = (GdkColor*)g_malloc0(sizeof(color));
                *configs.default_foreground_color = color;
            }
        }
        g_free(color_str);
    } else if (!strcmp(name, "showScrollbar"))
        config_getboolean(&(p_cfg->show_scrollbar), ls, index);
    else if (!strcmp(name, "transparentBackground"))
        config_getboolean(&(p_cfg->transparent_background), ls, index);
    else if (!strcmp(name, "transparentSaturation"))
        config_getdouble(&(p_cfg->transparent_saturation), ls, index);
    else if (!strcmp(name, "hideSingleTab"))
        config_getboolean(&(p_cfg->hide_single_tab), ls, index);
    else if (!strcmp(name, "scrollbackLines"))
        config_getuint(&(p_cfg->scrollback_lines), ls, index);
    else if (!strcmp(name, "allowChangingTitle"))
        config_getboolean(&(p_cfg->allow_changing_title), ls, index);
    else if (!strcmp(name, "geometry"))
    {
        gchar* geometry_str = NULL;
        config_getstring(&geometry_str, ls, index);
        if (geometry_str)
        {
            uint cols, rows;
            int tmp1, tmp2;
            XParseGeometry(geometry_str, &tmp1, &tmp2, &cols, &rows);
            if ((cols != 0) && (rows != 0))
            {
                p_cfg->cols = cols;
                p_cfg->rows = rows;
            }
        }
        g_free(geometry_str);
    }
    /*else if (!strcmp(name, ""))*/
        /*config_get(&(p_cfg->), ls, index);*/

}
static void load_init(const gchar* initFile)
{
    TRACE_FUNC;
    gchar* fullPath = NULL;
    if (initFile)
    {
        fullPath = g_strdup(initFile);
    }
    else
    {
        const gchar *configFile = "init.lua";
        const gchar *configHome = g_getenv("XDG_CONFIG_HOME");
        if (configHome)
            fullPath = g_strdup_printf("%s/termit/%s", configHome, configFile);
        else
        {
            fullPath = g_strdup_printf("%s/.config/termit/%s", g_getenv("HOME"), configFile);
        }
    }
    
    int s = luaL_loadfile(L, fullPath);
    termit_report_lua_error(s);
    g_free(fullPath);

    s = lua_pcall(L, 0, LUA_MULTRET, 0);
    termit_report_lua_error(s);

}

static const gchar* termit_init_file = NULL;

void termit_load_lua_config()
{
    load_init(termit_init_file);

    trace_menus(configs.user_menus);
    trace_menus(configs.user_popup_menus);
    
}

void termit_lua_init(const gchar* initFile)
{
    L = luaL_newstate();
    luaL_openlibs(L);

    termit_init_file = initFile;
    termit_init_lua_api();
    termit_load_lua_config();
}

