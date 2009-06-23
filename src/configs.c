#include <gdk/gdk.h>

#include "termit.h"
#include "keybindings.h"
#include "configs.h"
#include "lua_api.h"

struct Configs configs = {0};

void trace_configs()
{
#ifdef DEBUG
    TRACE_MSG("");
    TRACE("     default_window_title    = %s", configs.default_window_title);
    TRACE("     default_tab_name        = %s", configs.default_tab_name);
    TRACE("     default_encoding        = %s", configs.default_encoding);
    TRACE("     default_word_chars      = %s", configs.default_word_chars);
    TRACE("     show_scrollbar          = %d", configs.show_scrollbar);
    TRACE("     hide_menubar            = %d", configs.hide_menubar);
    TRACE("     fill_tabbar             = %d", configs.fill_tabbar);
    TRACE("     hide_single_tab         = %d", configs.hide_single_tab);
    TRACE("     scrollback_lines        = %d", configs.scrollback_lines);
    TRACE("     cols x rows             = %d x %d", configs.cols, configs.rows);
    TRACE("     allow_changing_title    = %d", configs.allow_changing_title);
    TRACE("     audible_bell            = %d", configs.audible_bell);
    TRACE("     visible_bell            = %d", configs.visible_bell);
    TRACE("     get_window_title_callback= %d", configs.get_window_title_callback);
    TRACE("     get_tab_title_callback  = %d", configs.get_tab_title_callback);
    TRACE("     style:");
    TRACE("       font_name             = %s", configs.style.font_name);
    TRACE("       foreground_color      = (%d,%d,%d)", 
                configs.style.foreground_color.red,
                configs.style.foreground_color.green,
                configs.style.foreground_color.blue);
    TRACE("       background_color      = (%d,%d,%d)", 
                configs.style.background_color.red,
                configs.style.background_color.green,
                configs.style.background_color.blue);
    TRACE("       transparency          = %f", configs.style.transparency);
    TRACE_MSG("");
#endif 
}

void termit_set_default_options()
{
    configs.default_window_title = g_strdup("Termit");
    configs.default_tab_name = g_strdup("Terminal");
    termit_style_init(&configs.style);
    configs.default_command = g_strdup(g_getenv("SHELL"));
    configs.default_encoding = g_strdup("UTF-8");
    configs.default_word_chars = g_strdup("-A-Za-z0-9,./?%&#_~");
    configs.scrollback_lines = 4096;
    configs.cols = 80;
    configs.rows = 24;

    configs.user_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.user_popup_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.key_bindings = g_array_new(FALSE, TRUE, sizeof(struct KeyBinding));
    configs.mouse_bindings = g_array_new(FALSE, TRUE, sizeof(struct MouseBinding));
    configs.encodings = g_array_new(FALSE, TRUE, sizeof(gchar*));
    configs.matches = g_array_new(FALSE, TRUE, sizeof(struct Match));

    configs.hide_single_tab = FALSE;
    configs.show_scrollbar = TRUE;
    configs.fill_tabbar = FALSE;
    configs.hide_menubar = FALSE;
    configs.allow_changing_title = FALSE;
    configs.visible_bell = FALSE;
    configs.audible_bell = FALSE;
    configs.get_window_title_callback = 0;
    configs.get_tab_title_callback = 0;
    configs.kb_policy = TermitKbUseKeysym;
}

static void free_menu(GArray* menus)
{
    gint i = 0;
    for (; i<menus->len; ++i) {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        gint j = 0;
        for (; j<um->items->len; ++j) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, j);
            g_free(umi->name);
            g_free(umi->userFunc);
        }
        g_free(um->name);
        g_array_free(um->items, TRUE);
    }
}

void termit_deinit_config()
{
    g_free(configs.default_window_title);
    g_free(configs.default_tab_name);
    termit_style_free(&configs.style);
    g_free(configs.default_command);
    g_free(configs.default_encoding);
    g_free(configs.default_word_chars);

    gint i = 0;
    for (; i<configs.encodings->len; ++i)
        g_free(g_array_index(configs.encodings, gchar*, i));
    g_array_free(configs.encodings, TRUE);
    
    free_menu(configs.user_menus);
    g_array_free(configs.user_menus, TRUE);
    free_menu(configs.user_popup_menus);
    g_array_free(configs.user_popup_menus, TRUE);

    // name and default_binding are static (e.g. can be in readonly mempage)
    // TODO luaL_unref all callbacks
    g_array_free(configs.key_bindings, TRUE);
    g_array_free(configs.mouse_bindings, TRUE);
    i = 0;
    for (; i<configs.matches->len; ++i) {
        struct Match* match = &g_array_index(configs.matches, struct Match, i);
        g_regex_unref(match->regex);
        g_free(match->pattern);
    }
    g_array_free(configs.matches, TRUE);
    
    termit_lua_unref(&configs.get_window_title_callback);
    termit_lua_unref(&configs.get_tab_title_callback);
}

