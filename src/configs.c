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
#include <gdk/gdk.h>

#include "termit.h"
#include "keybindings.h"
#include "configs.h"
#include "lua_api.h"

struct Configs configs = {};

static struct {
    const char* name;
    VteTerminalEraseBinding val;
} erase_bindings[] = {
    {"Auto", VTE_ERASE_AUTO},
    {"AsciiBksp", VTE_ERASE_ASCII_BACKSPACE},
    {"AsciiDel", VTE_ERASE_ASCII_DELETE},
    {"EraseDel", VTE_ERASE_DELETE_SEQUENCE},
    {"EraseTty", VTE_ERASE_TTY}
};
static guint EraseBindingsSz = sizeof(erase_bindings)/sizeof(erase_bindings[0]);

const char* termit_erase_binding_to_string(VteTerminalEraseBinding val)
{
    return erase_bindings[val].name;
}
VteTerminalEraseBinding termit_erase_binding_from_string(const char* str)
{
    guint i = 0;
    for (; i < EraseBindingsSz; ++i) {
        if (strcmp(str, erase_bindings[i].name) == 0) {
            return erase_bindings[i].val;
        }
    }
    ERROR("not found binding for [%s], using Auto", str);
    return VTE_ERASE_AUTO;
}

void termit_config_trace()
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
    TRACE("     backspace               = %s", termit_erase_binding_to_string(configs.default_bksp));
    TRACE("     delete                  = %s", termit_erase_binding_to_string(configs.default_delete));
    TRACE("     allow_changing_title    = %d", configs.allow_changing_title);
    TRACE("     audible_bell            = %d", configs.audible_bell);
    TRACE("     visible_bell            = %d", configs.visible_bell);
    TRACE("     get_window_title_callback= %d", configs.get_window_title_callback);
    TRACE("     get_tab_title_callback  = %d", configs.get_tab_title_callback);
    TRACE("     get_statusbar_callback  = %d", configs.get_statusbar_callback);
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
    TRACE("       image_file            = %s", configs.style.image_file);
    TRACE_MSG("");
#endif 
}

void termit_configs_set_defaults()
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
    configs.default_bksp = VTE_ERASE_AUTO;
    configs.default_delete = VTE_ERASE_AUTO;

    configs.user_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.user_popup_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.key_bindings = g_array_new(FALSE, TRUE, sizeof(struct KeyBinding));
    configs.mouse_bindings = g_array_new(FALSE, TRUE, sizeof(struct MouseBinding));
    configs.matches = g_array_new(FALSE, TRUE, sizeof(struct Match));

    configs.hide_single_tab = FALSE;
    configs.show_scrollbar = TRUE;
    configs.fill_tabbar = FALSE;
    configs.hide_menubar = FALSE;
    configs.allow_changing_title = FALSE;
    configs.visible_bell = FALSE;
    configs.audible_bell = FALSE;
    configs.urgency_on_bell = FALSE;
    configs.get_window_title_callback = 0;
    configs.get_tab_title_callback = 0;
    configs.get_statusbar_callback = 0;
    configs.kb_policy = TermitKbUseKeysym;
}

static void free_menu(GArray* menus)
{
    guint i = 0;
    for (; i<menus->len; ++i) {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        guint j = 0;
        for (; j<um->items->len; ++j) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, j);
            g_free(umi->name);
            g_free(umi->accel);
            termit_lua_unref(&umi->lua_callback);
        }
        g_free(um->name);
        g_array_free(um->items, TRUE);
    }
}

void termit_config_deinit()
{
    g_free(configs.default_window_title);
    g_free(configs.default_tab_name);
    termit_style_free(&configs.style);
    g_free(configs.default_command);
    g_free(configs.default_encoding);
    g_free(configs.default_word_chars);

    free_menu(configs.user_menus);
    g_array_free(configs.user_menus, TRUE);
    free_menu(configs.user_popup_menus);
    g_array_free(configs.user_popup_menus, TRUE);

    // name and default_binding are static (e.g. can be in readonly mempage)
    guint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        termit_lua_unref(&kb->lua_callback);
    }
    g_array_free(configs.key_bindings, TRUE);

    i = 0;
    for (; i<configs.mouse_bindings->len; ++i) {
        struct MouseBinding* mb = &g_array_index(configs.mouse_bindings, struct MouseBinding, i);
        termit_lua_unref(&mb->lua_callback);
    }
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
    termit_lua_unref(&configs.get_statusbar_callback);
}

