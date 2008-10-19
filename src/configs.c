
#include "termit.h"
#include "keybindings.h"
#include "configs.h"

struct Configs configs = {0};

void trace_configs()
{
#ifdef DEBUG
    TRACE_MSG("");
    TRACE("     default_window_title    = %s", configs.default_window_title);
    TRACE("     default_tab_name        = %s", configs.default_tab_name);
    TRACE("     default_encoding        = %s", configs.default_encoding);
    TRACE("     default_word_chars      = %s", configs.default_word_chars);
    TRACE("     default_font            = %s", configs.default_font);
    TRACE("     show_scrollbar          = %d", configs.show_scrollbar);
    TRACE("     transparent_background  = %d", configs.transparent_background);
    TRACE("     transparent_saturation  = %f", configs.transparent_saturation);
    TRACE("     hide_single_tab         = %d", configs.hide_single_tab);
    TRACE("     allow_changing_title    = %d", configs.allow_changing_title);
    TRACE("     scrollback_lines        = %d", configs.scrollback_lines);
    TRACE("     cols x rows             = %d x %d", configs.cols, configs.rows);
    TRACE_MSG("");
#endif 
}

void termit_set_default_options()
{
    configs.default_window_title = g_strdup("Termit");
    configs.default_tab_name = g_strdup("Terminal");
    configs.default_font = g_strdup("Monospace 10");
    configs.default_command = g_strdup(g_getenv("SHELL"));
    configs.default_encoding = g_strdup("UTF-8");
    configs.default_word_chars = g_strdup("-A-Za-z0-9,./?%&#_~");
    configs.scrollback_lines = 4096;
    configs.transparent_background = FALSE;
    configs.transparent_saturation = 0.2;
    configs.cols = 80;
    configs.rows = 24;

    configs.user_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.user_popup_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.key_bindings = g_array_new(FALSE, TRUE, sizeof(struct KeyBindging));
    configs.encodings = g_array_new(FALSE, TRUE, sizeof(gchar*));

    configs.hide_single_tab = FALSE;
    configs.show_scrollbar = TRUE;
    configs.allow_changing_title = FALSE;
    configs.kb_policy = TermitKbUseKeysym;
}

void termit_set_defaults()
{
    termit_set_default_options();
    termit_set_default_keybindings();
}

void termit_free_colors()
{
#if 0
    gint i=0;
    for (; i<configs.colors.palette_size; ++i)
        gdk_color_free(configs.colors.palette[i]);
    gdk_color_free(configs.colors.foreground);
    gdk_color_free(configs.colors.background);
    struct TermitColors tmp = {0};
    configs.colors = tmp;
#endif
}

static void free_menu(GArray* menus)
{
    gint i=0;
    for (; i<menus->len; ++i)
    {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        gint j = 0;
        for (; j<um->items->len; ++j)
        {
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
    g_free(configs.default_font);
    g_free(configs.default_command);
    g_free(configs.default_encoding);
    g_free(configs.default_word_chars);

    gint i=0;
    for (; i<configs.encodings->len; ++i)
        g_free(g_array_index(configs.encodings, gchar*, i));
    g_array_free(configs.encodings, TRUE);
    
    free_menu(configs.user_menus);
    g_array_free(configs.user_menus, TRUE);
    free_menu(configs.user_popup_menus);
    g_array_free(configs.user_popup_menus, TRUE);

    termit_free_colors();

    // name and default_binding are static (e.g. can be in readonly mempage)
    g_array_free(configs.key_bindings, TRUE);
}

