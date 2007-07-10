#ifndef CALLBACKS_H
#define CALLBACKS_H


gboolean termit_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
void termit_destroy(GtkWidget *widget, gpointer data);
void termit_child_exited();
void termit_eof();
void termit_title_changed();
gboolean termit_popup(GtkWidget *, GdkEvent *);
gboolean termit_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
void termit_set_encoding(GtkWidget *, void *);
void termit_new_tab();
void termit_prev_tab();
void termit_next_tab();
void termit_paste();
void termit_copy();
void termit_close_tab();
void termit_set_tab_name();
void termit_select_font();
void termit_menu_exit();
void termit_switch_page(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data);
void termit_cb_bookmarks_changed(GtkComboBox *widget, gpointer user_data);
void termit_append_tab();
void termit_set_font();
void termit_set_statusbar_encoding(gint page);

                                            
                                            
#endif /* CALLBACKS_H */

