
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "utils.h"
#include "callbacks.h"
#include "configs.h"
#include "keybindings.h"
#include "sessions.h"

extern struct TermitData termit;
extern struct Configs configs;

static gboolean confirm_exit()
{
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) <= 1)
        return FALSE;

    GtkWidget *dlg = gtk_message_dialog_new(
        GTK_WINDOW(termit.main_window), 
        GTK_DIALOG_MODAL, 
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO, _("Several tabs are opened.\nClose anyway?"));
    gint response = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    if (response == GTK_RESPONSE_YES)
        return FALSE;
    else
        return TRUE;
}

static void termit_quit()
{
    while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) > 0)
        termit_del_tab();
    
    g_strfreev(configs.encodings);
    g_array_free(configs.bookmarks, TRUE);
    g_array_free(configs.key_bindings, TRUE);
    pango_font_description_free(termit.font);

    gtk_main_quit();
}

gboolean termit_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return confirm_exit();
}

void termit_destroy(GtkWidget *widget, gpointer data)
{
    termit_quit();
}

void termit_toggle_scrollbar()
{
    TRACE_MSG(__FUNCTION__);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);

    if (pTab->scrollbar_is_shown)
        gtk_widget_hide(GTK_WIDGET(pTab->scrollbar));
    else
        gtk_widget_show(GTK_WIDGET(pTab->scrollbar));
    pTab->scrollbar_is_shown = !pTab->scrollbar_is_shown;
}

void termit_child_exited()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);

    TRACE("waiting for pid %d", pTab->pid);
    
    int status = 0;
    waitpid(pTab->pid, &status, WNOHANG);
    /* TODO: check wait return */    

    termit_del_tab();
    
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 0)
        termit_quit();
}

void termit_eof()
{
}

void termit_title_changed()
{
}

gboolean termit_popup(GtkWidget *widget, GdkEvent *event)
{
    GtkMenu *menu;
    GdkEventButton *event_button;

    menu = GTK_MENU (widget);

    if (event->type == GDK_BUTTON_PRESS)
    {        
        event_button = (GdkEventButton *) event;
        if (event_button->button == 3)
        {
            gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                              event_button->button, event_button->time);
            return TRUE;
        }
    }

    return FALSE;
}

gboolean termit_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    return termit_process_key(event);
}

void termit_set_encoding(GtkWidget *widget, void *data)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), (gchar*)data);
    g_free(pTab->encoding);
    pTab->encoding = g_strdup((gchar*)data);
    termit_set_statusbar_encoding(-1);
}

void termit_prev_tab()
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    if (index == -1)
        return;
    if (index)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index - 1);        
    else
        gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), 
            gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) - 1);
}

void termit_next_tab()
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    if (index == -1)
        return;
    if (index == (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) - 1))
        gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), 0);
    else
        gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index + 1);
}

void termit_paste()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    vte_terminal_paste_clipboard(VTE_TERMINAL(pTab->vte));
}

void termit_copy()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    vte_terminal_copy_clipboard(VTE_TERMINAL(pTab->vte));
}

void termit_close_tab()
{
    termit_del_tab();
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 0)
        termit_quit();
}

static gboolean dlg_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval)
    {
    case GDK_Return:
        g_signal_emit_by_name(GTK_OBJECT(widget), "response", GTK_RESPONSE_ACCEPT, NULL);
        break;
    case GDK_Escape:
        g_signal_emit_by_name(GTK_OBJECT(widget), "response", GTK_RESPONSE_REJECT, NULL);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void termit_set_tab_name()
{
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        _("Tab name"), 
        GTK_WINDOW(termit.main_window), 
        GTK_DIALOG_MODAL, 
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);

    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    GtkWidget *label = gtk_label_new(_("Tab name"));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(
        GTK_ENTRY(entry), 
        gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(termit.notebook), pTab->hbox));
    
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE, 5);
    
    g_signal_connect(G_OBJECT(dlg), "key-press-event", G_CALLBACK(dlg_key_press), dlg);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, 10);
    gtk_widget_show_all(dlg);
    
    if (GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dlg)))
        gtk_label_set_text(GTK_LABEL(pTab->tab_name), gtk_entry_get_text(GTK_ENTRY(entry)));
    
    gtk_widget_destroy(dlg);
}

void termit_select_font()
{
    GtkWidget *dlg = gtk_font_selection_dialog_new(_("Select font"));
    gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dlg), 
                                            pango_font_description_to_string(termit.font));

    if (GTK_RESPONSE_OK == gtk_dialog_run(GTK_DIALOG(dlg)))
    {
        pango_font_description_free(termit.font);
        termit.font = pango_font_description_from_string(gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dlg)));
        termit_set_font();
    }

    gtk_widget_destroy(dlg);
}

void termit_new_tab()
{
    termit_append_tab();
}

void termit_menu_exit()
{
    if (confirm_exit() == FALSE)
        termit_quit();
}

void termit_switch_page(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{   
    TERMIT_GET_TAB_BY_INDEX(pTab, page_num);
    // it seems that set_active eventually calls toggle callback
    ((GtkCheckMenuItem*)termit.mi_show_scrollbar)->active = pTab->scrollbar_is_shown;

    termit_set_statusbar_encoding(page_num);
}

gboolean termit_bookmark_selected(GtkComboBox *widget, GdkEventButton *event, gpointer user_data)
{
    if (event->button == 2)
        termit_append_tab();
        
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX2(pTab, page, FALSE);
    gchar* cmd = g_strdup_printf("cd %s\n", ((struct Bookmark*)user_data)->path);
    GString* cmdStr = g_string_new(cmd);
    vte_terminal_feed_child(VTE_TERMINAL(pTab->vte), cmdStr->str, cmdStr->len);

    g_string_free(cmdStr, TRUE);
    g_free(cmd);

    if (event->button == 2)
        gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(termit.notebook), pTab->hbox,
            ((struct Bookmark*)user_data)->name);
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);
    return TRUE;
}

gint termit_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
    if (event->type == GDK_2BUTTON_PRESS)
        termit_append_tab();

    return FALSE;
}

gchar* termit_get_xdg_data_path()
{
    gchar* fullPath = NULL;
    const gchar *dataHome = g_getenv("XDG_DATA_HOME");
    if (dataHome)
        fullPath = g_strdup_printf("%s/termit", dataHome);
    else
    {
        fullPath = g_strdup_printf("%s/.local/share/termit", g_getenv("HOME"));
    }
    TRACE("XDG_DATA_PATH=%s", fullPath);
    return fullPath;
}

void termit_on_save_session()
{
    gchar* fullPath = termit_get_xdg_data_path();
    
    GtkWidget* dlg = gtk_file_chooser_dialog_new(
        _("Save session"), 
        GTK_WINDOW(termit.main_window), 
        GTK_FILE_CHOOSER_ACTION_SAVE, 
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), fullPath);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "New session");

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_destroy(dlg);
        g_free(fullPath);
        return;
    }

    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    TRACE("saving session to file %s", filename);
    FILE* fd = g_fopen(filename, "w");
    if ((intptr_t)fd == -1)
    {
        gtk_widget_destroy(dlg);
        g_free(filename);
        g_free(fullPath);
        return;
    }
    
    GKeyFile *kf = g_key_file_new();
    guint pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    g_key_file_set_integer(kf, "session", "tab_count", pages);
    
    guint i = 0;
    for (; i < pages; ++i)
    {
        gchar* groupName = g_strdup_printf("tab%d", i);
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        g_key_file_set_string(kf, groupName, "tab_name", gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
        g_key_file_set_string(kf, groupName, "encoding", pTab->encoding);
        gchar* working_dir = termit_get_pid_dir(pTab->pid);
        g_key_file_set_string(kf, groupName, "working_dir", working_dir);
        g_free(working_dir);
        g_free(groupName);
    }
    gchar* data = g_key_file_to_data(kf, NULL, NULL);
    g_fprintf(fd, data);
    fclose(fd);
    g_key_file_free(kf);

    g_free(filename);
    gtk_widget_destroy(dlg);
    g_free(fullPath);
}

void termit_on_load_session()
{
    gchar* fullPath = termit_get_xdg_data_path();

    GtkWidget* dlg = gtk_file_chooser_dialog_new(
        _("Open session"), 
        GTK_WINDOW(termit.main_window), 
        GTK_FILE_CHOOSER_ACTION_OPEN, 
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), fullPath);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT)
        goto free_dlg;

    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    termit_load_session(filename);
    g_free(filename);
free_dlg:
    gtk_widget_destroy(dlg);
    g_free(fullPath);
}

