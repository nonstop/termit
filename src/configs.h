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

#ifndef TERMIT_CONFIGS_H
#define TERMIT_CONFIGS_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "termit_style.h"

enum TermitKbPolicy {TermitKbUseKeycode = 1, TermitKbUseKeysym = 2};

struct Configs
{
    gchar* default_window_title;
    gchar* default_tab_name;
    gchar* default_command;
    gchar* default_encoding;
    gchar* default_word_char_exceptions;
    guint scrollback_lines;
    guint cols;
    guint rows;
    VteEraseBinding default_bksp;
    VteEraseBinding default_delete;
    VteCursorBlinkMode default_blink;
    VteCursorShape default_shape;
    GArray* user_menus;         // UserMenu
    GArray* user_popup_menus;   // UserMenu
    GArray* key_bindings;       // KeyBinding
    GArray* mouse_bindings;     // MouseBinding
    GArray* matches;            // Match
    gboolean start_maximized;
    gboolean hide_titlebar_when_maximized;
    gboolean hide_single_tab;
    gboolean show_scrollbar;
    gboolean hide_menubar;
    gboolean hide_tabbar;
    gboolean fill_tabbar;
    gboolean show_border;
    gboolean urgency_on_bell;
    gboolean allow_changing_title;
    gboolean audible_bell;
    gboolean scroll_on_output;
    gboolean scroll_on_keystroke;
    int get_window_title_callback;
    int get_tab_title_callback;
    int get_statusbar_callback;
    enum TermitKbPolicy kb_policy;
    GtkPositionType tab_pos;
    struct TermitStyle style;
    GArray* default_tabs;       // TabInfo
};

struct Match
{
    gchar* pattern;
    VteRegex* regex;
    guint32 flags;
    int tag;
    int lua_callback;
};
struct UserMenuItem
{
    gchar* name;
    gchar* accel;
    int lua_callback;
};
struct UserMenu
{
    gchar* name;
    GArray* items; // UserMenuItem
};

extern struct Configs configs;

void termit_config_deinit();
void termit_configs_set_defaults();
void termit_config_load();

void termit_config_trace();
void termit_keys_trace();

const char* termit_erase_binding_to_string(VteEraseBinding val);
VteEraseBinding termit_erase_binding_from_string(const char* str);

const char* termit_cursor_blink_mode_to_string(VteCursorBlinkMode val);
VteCursorBlinkMode termit_cursor_blink_mode_from_string(const char* str);

const char* termit_cursor_shape_to_string(VteCursorShape val);
VteCursorShape termit_cursor_shape_from_string(const char* str);

#define TERMIT_USER_MENU_ITEM_DATA "termit.umi_data"
#define TERMIT_TAB_DATA "termit.tab_data"

#endif /* TERMIT_CONFIGS_H */
