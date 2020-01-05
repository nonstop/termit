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

#ifndef TERMIT_CALLBACKS_H
#define TERMIT_CALLBACKS_H

gboolean termit_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
void termit_on_destroy(GtkWidget *widget, gpointer data);
gboolean termit_on_popup(GtkWidget *, GdkEvent *);
gboolean termit_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean termit_on_focus(GtkWidget *widget, GtkDirectionType arg1, gpointer user_data);
void termit_on_beep(VteTerminal *vte, gpointer user_data);
void termit_on_edit_preferences();
void termit_on_set_tab_name();
void termit_on_toggle_scrollbar();
void termit_on_child_exited(VteTerminal*, gint, gpointer);
void termit_on_exit();
void termit_on_switch_page(GtkNotebook *notebook, gpointer arg, guint page, gpointer user_data);
void termit_on_menu_item_selected(GtkWidget *widget, void *data);
gint termit_on_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data);
void termit_on_save_session();
void termit_on_load_session();
void termit_on_tab_title_changed(VteTerminal *vte, gpointer user_data);
void termit_on_toggle_search(GtkToggleButton*, gpointer);
void termit_on_find_next(GtkButton*, gpointer);
void termit_on_find_prev(GtkButton*, gpointer);
gboolean termit_on_search_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

#endif /* TERMIT_CALLBACKS_H */
