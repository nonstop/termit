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

#ifndef TERMIT_CORE_API_H
#define TERMIT_CORE_API_H

#include "termit.h"
#include "configs.h"

void termit_reconfigure();
void termit_after_show_all();

void termit_append_tab();
void termit_append_tab_with_command(gchar** argv);
void termit_append_tab_with_details(const struct TabInfo*);

void termit_activate_tab(gint tab_index);
void termit_prev_tab();
void termit_next_tab();
void termit_paste();
void termit_copy();
gchar* termit_get_selection();
void termit_search_find_next();
void termit_search_find_prev();
void termit_for_each_row(int lua_callback);
void termit_for_each_visible_row(int lua_callback);
void termit_del_tab_n(gint page);
void termit_close_tab();
void termit_toggle_menubar();
void termit_toggle_tabbar();
void termit_toggle_search();
void termit_set_window_title(const gchar* title);
void termit_set_statusbar_message(guint page);
void termit_set_encoding(const gchar* encoding);
void termit_quit();

void termit_tab_feed(struct TermitTab* pTab, const gchar* data);
void termit_tab_feed_child(struct TermitTab* pTab, const gchar* data);
void termit_tab_set_font(struct TermitTab* pTab, const gchar* font_name);
void termit_tab_set_font_by_index(gint tab_index, const gchar* font_name);
void termit_tab_apply_colors(struct TermitTab* pTab);
void termit_tab_set_color_foreground(struct TermitTab* pTab, const GdkRGBA* p_color);
void termit_tab_set_color_background(struct TermitTab* pTab, const GdkRGBA* p_color);
void termit_tab_set_color_foreground_by_index(gint tab_index, const GdkRGBA*);
void termit_tab_set_color_background_by_index(gint tab_index, const GdkRGBA*);
void termit_tab_set_title(struct TermitTab* pTab, const gchar* title);
void termit_tab_set_audible_bell(struct TermitTab* pTab, gboolean audible_bell);
void termit_tab_set_pos(struct TermitTab* pTab, int newPos);

int termit_get_current_tab_index();
gchar* termit_get_pid_dir(pid_t pid);

/**
 * function to switch key processing policy
 * keycodes - kb layout independent
 * keysyms - kb layout dependent
 * */
void termit_set_kb_policy(enum TermitKbPolicy kbp);
void termit_set_show_scrollbar_signal(GtkWidget* menuItem, gpointer pHanderId);

#endif /* TERMIT_CORE_API_H */
