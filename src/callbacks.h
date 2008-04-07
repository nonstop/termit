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
gboolean termit_bookmark_selected(GtkComboBox *widget, GdkEventButton *event, gpointer user_data);
void termit_append_tab();
void termit_append_tab_with_command(const gchar* command);
void termit_append_tab_with_details(const gchar* tab_name, const gchar* shell_cmd, const gchar* working_dir, const gchar* encoding);
gchar* termit_get_pid_dir(pid_t pid);
void termit_del_tab();

void termit_set_font();
void termit_set_statusbar_encoding(gint page);
gint termit_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data);
void termit_on_save_session();
void termit_on_load_session();
                                            
                                            
#endif /* CALLBACKS_H */

