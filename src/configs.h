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

#ifndef CONFIGS_H
#define CONFIGS_H

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
    gchar* default_word_chars;
    guint scrollback_lines;
    guint cols;
    guint rows;
    VteTerminalEraseBinding default_bksp;
    VteTerminalEraseBinding default_delete;
    GArray* user_menus;         // UserMenu
    GArray* user_popup_menus;   // UserMenu
    GArray* key_bindings;       // KeyBinding
    GArray* mouse_bindings;     // MouseBinding
    GArray* matches;            // Match
    gboolean hide_single_tab;
    gboolean show_scrollbar;
    gboolean hide_menubar;
    gboolean hide_tabbar;
    gboolean fill_tabbar;
    gboolean urgency_on_bell;
    gboolean allow_changing_title;
    gboolean audible_bell;
    gboolean visible_bell;
    int get_window_title_callback;
    int get_tab_title_callback;
    int get_statusbar_callback;
    enum TermitKbPolicy kb_policy;
    struct TermitStyle style;
    GArray* default_tabs;       // TabInfo
};

struct Match
{
    gchar* pattern;
    GRegex* regex;
    GRegexMatchFlags flags;
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

const char* termit_erase_binding_to_string(VteTerminalEraseBinding val);
VteTerminalEraseBinding termit_erase_binding_from_string(const char* str);

#define TERMIT_USER_MENU_ITEM_DATA "termit.umi_data"
#define TERMIT_TAB_DATA "termit.tab_data"

#endif /* CONFIGS_H */

