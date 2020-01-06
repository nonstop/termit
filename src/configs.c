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

#include <string.h>
#include <gdk/gdk.h>

#include "termit.h"
#include "keybindings.h"
#include "configs.h"
#include "lua_api.h"

struct Configs configs = {};

static struct {
    const char* name;
    VteEraseBinding val;
} erase_bindings[] = {
    {"Auto", VTE_ERASE_AUTO},
    {"AsciiBksp", VTE_ERASE_ASCII_BACKSPACE},
    {"AsciiDel", VTE_ERASE_ASCII_DELETE},
    {"EraseDel", VTE_ERASE_DELETE_SEQUENCE},
    {"EraseTty", VTE_ERASE_TTY}
};
static guint EraseBindingsSz = sizeof(erase_bindings)/sizeof(erase_bindings[0]);

const char* termit_erase_binding_to_string(VteEraseBinding val)
{
    return erase_bindings[val].name;
}

VteEraseBinding termit_erase_binding_from_string(const char* str)
{
    guint i = 0;
    for (; i < EraseBindingsSz; ++i) {
        if (strcmp(str, erase_bindings[i].name) == 0) {
            return erase_bindings[i].val;
        }
    }
    ERROR("unknown erase binding [%s], using Auto", str);
    return VTE_ERASE_AUTO;
}

static struct {
    const char* name;
    VteCursorBlinkMode val;
} cursor_blink_modes[] = {
    {"System", VTE_CURSOR_BLINK_SYSTEM},
    {"BlinkOn", VTE_CURSOR_BLINK_ON},
    {"BlinkOff", VTE_CURSOR_BLINK_OFF}
};
static guint BlinkModesSz = sizeof(cursor_blink_modes)/sizeof(cursor_blink_modes[0]);

const char* termit_cursor_blink_mode_to_string(VteCursorBlinkMode val)
{
    return cursor_blink_modes[val].name;
}

VteCursorBlinkMode termit_cursor_blink_mode_from_string(const char* str)
{
    guint i = 0;
    for (; i < BlinkModesSz; ++i) {
        if (strcmp(str, cursor_blink_modes[i].name) == 0) {
            return cursor_blink_modes[i].val;
        }
    }
    ERROR("unknown blink mode [%s], using System", str);
    return VTE_CURSOR_BLINK_SYSTEM;
}

static struct {
    const char* name;
    VteCursorShape val;
} cursor_shapes[] = {
    {"Block", VTE_CURSOR_SHAPE_BLOCK},
    {"Ibeam", VTE_CURSOR_SHAPE_IBEAM},
    {"Underline", VTE_CURSOR_SHAPE_UNDERLINE}
};
static guint ShapesSz = sizeof(cursor_shapes)/sizeof(cursor_shapes[0]);

const char* termit_cursor_shape_to_string(VteCursorShape val)
{
    return cursor_shapes[val].name;
}

VteCursorShape termit_cursor_shape_from_string(const char* str)
{
    guint i = 0;
    for (; i < ShapesSz; ++i) {
        if (strcmp(str, cursor_shapes[i].name) == 0) {
            return cursor_shapes[i].val;
        }
    }
    ERROR("unknown cursor shape [%s], using Block", str);
    return VTE_CURSOR_SHAPE_BLOCK;
}

void termit_config_trace()
{
#ifdef DEBUG
    TRACE("   default_window_title          = %s", configs.default_window_title);
    TRACE("   default_tab_name              = %s", configs.default_tab_name);
    TRACE("   default_encoding              = %s", configs.default_encoding);
    TRACE("   default_word_char_exceptions  = %s", configs.default_word_char_exceptions);
    TRACE("   show_scrollbar                = %d", configs.show_scrollbar);
    TRACE("   hide_menubar                  = %d", configs.hide_menubar);
    TRACE("   hide_tabbar                   = %d", configs.hide_tabbar);
    TRACE("   fill_tabbar                   = %d", configs.fill_tabbar);
    TRACE("   show_border                   = %d", configs.show_border);
    TRACE("   hide_single_tab               = %d", configs.hide_single_tab);
    TRACE("   start_maximized               = %d", configs.start_maximized);
    TRACE("   hide_titlebar_when_maximized  = %d", configs.hide_titlebar_when_maximized);
    TRACE("   scrollback_lines              = %d", configs.scrollback_lines);
    TRACE("   cols x rows                   = %d x %d", configs.cols, configs.rows);
    TRACE("   backspace                     = %s", termit_erase_binding_to_string(configs.default_bksp));
    TRACE("   delete                        = %s", termit_erase_binding_to_string(configs.default_delete));
    TRACE("   blink                         = %s", termit_cursor_blink_mode_to_string(configs.default_blink));
    TRACE("   shape                         = %s", termit_cursor_shape_to_string(configs.default_shape));
    TRACE("   allow_changing_title          = %d", configs.allow_changing_title);
    TRACE("   audible_bell                  = %d", configs.audible_bell);
    TRACE("   scroll_on_output              = %d", configs.scroll_on_output);
    TRACE("   scroll_on_keystroke           = %d", configs.scroll_on_keystroke);
    TRACE("   get_window_title_callback     = %d", configs.get_window_title_callback);
    TRACE("   get_tab_title_callback        = %d", configs.get_tab_title_callback);
    TRACE("   get_statusbar_callback        = %d", configs.get_statusbar_callback);
    TRACE("   kb_policy                     = %d", configs.kb_policy);
    TRACE("   tab_pos                       = %d", configs.tab_pos);
    TRACE("   style:");
    TRACE("     font_name                   = %s", configs.style.font_name);
    if (configs.style.foreground_color) {
        gchar* tmpStr = gdk_rgba_to_string(configs.style.foreground_color);
        TRACE("     foreground_color            = %s", tmpStr);
        g_free(tmpStr);
    }
    if (configs.style.background_color) {
        gchar* tmpStr = gdk_rgba_to_string(configs.style.background_color);
        TRACE("     background_color            = %s", tmpStr);
        g_free(tmpStr);
    }
#endif
}

void termit_configs_set_defaults()
{
    configs.default_window_title = g_strdup("Termit");
    configs.default_tab_name = g_strdup("Terminal");
    termit_style_init(&configs.style);
    configs.default_command = g_strdup(g_getenv("SHELL"));
    configs.default_encoding = g_strdup("UTF-8");
    configs.default_word_char_exceptions = g_strdup("-A-Za-z0-9,./?%&#_~");
    configs.scrollback_lines = 4096;
    configs.cols = 80;
    configs.rows = 24;
    configs.default_bksp = VTE_ERASE_AUTO;
    configs.default_delete = VTE_ERASE_AUTO;
    configs.default_blink = VTE_CURSOR_BLINK_SYSTEM;
    configs.default_shape = VTE_CURSOR_SHAPE_BLOCK;

    configs.user_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.user_popup_menus = g_array_new(FALSE, TRUE, sizeof(struct UserMenu));
    configs.key_bindings = g_array_new(FALSE, TRUE, sizeof(struct KeyBinding));
    configs.mouse_bindings = g_array_new(FALSE, TRUE, sizeof(struct MouseBinding));
    configs.matches = g_array_new(FALSE, TRUE, sizeof(struct Match));

    configs.start_maximized = FALSE;
    configs.hide_titlebar_when_maximized = FALSE;
    configs.hide_single_tab = FALSE;
    configs.show_scrollbar = TRUE;
    configs.fill_tabbar = FALSE;
    configs.hide_menubar = FALSE;
    configs.hide_tabbar = FALSE;
    configs.show_border = TRUE;
    configs.allow_changing_title = FALSE;
    configs.audible_bell = FALSE;
    configs.urgency_on_bell = FALSE;
    configs.get_window_title_callback = 0;
    configs.get_tab_title_callback = 0;
    configs.get_statusbar_callback = 0;
    configs.kb_policy = TermitKbUseKeysym;
    configs.tab_pos = GTK_POS_TOP;
    configs.scroll_on_output = FALSE;
    configs.scroll_on_keystroke = TRUE;
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
    g_free(configs.default_word_char_exceptions);

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
        vte_regex_unref(match->regex);
        g_free(match->pattern);
    }
    g_array_free(configs.matches, TRUE);

    termit_lua_unref(&configs.get_window_title_callback);
    termit_lua_unref(&configs.get_tab_title_callback);
    termit_lua_unref(&configs.get_statusbar_callback);
}
