
#include <sys/wait.h>
#include <gdk/gdkkeysyms.h>

#include "utils.h"
#include "callbacks.h"
#include "configs.h"

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

static void termit_del_tab()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, page);
    
    g_free(tab.encoding);
    g_array_remove_index(termit.tabs, page);
            
    gtk_notebook_remove_page(GTK_NOTEBOOK(termit.notebook), page);
}

static void termit_quit()
{
    while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) > 0)
        termit_del_tab();
    
    g_array_free(termit.tabs, TRUE);
    g_strfreev(configs.encodings);
    g_array_free(configs.bookmarks, TRUE);
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

void termit_child_exited()
{
	gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
	struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, page);

	TRACE_STR("waiting for pid");
    TRACE_NUM(tab.pid);
	
    int status = 0;
	waitpid(tab.pid, &status, WNOHANG);
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
	unsigned int topage = 0;
	gint npages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));

//  Alt-left, Alt-right switch pages
    if (event->state & GDK_MOD1_MASK)
    {
        if (event->keyval == GDK_Left)
        {
            termit_prev_tab();
            return TRUE;
        }
        if (event->keyval == GDK_Right)
        {
            termit_next_tab();
            return TRUE;
        }
    }
//  Ctrl-Insert, Shift-Insert copy paste
    if (event->keyval == GDK_Insert)
    {
        if (event->state & GDK_SHIFT_MASK)
        {
            termit_paste();
            return TRUE;
        }
        if (event->state & GDK_CONTROL_MASK)
        {
            termit_copy();
            return TRUE;
        }
    }
//  Ctrl+t new tab, Ctrl+w close tab
    if (event->state & GDK_CONTROL_MASK)
    {
        if (event->keyval == GDK_t || event->keyval == GDK_T)
        {
            termit_append_tab();
            return TRUE;
        }
        if (event->keyval == GDK_w || event->keyval == GDK_W)
        {
            termit_close_tab();
            return TRUE;
        }
    }
	return FALSE;
}

void termit_set_encoding(GtkWidget *widget, void *data)
{
    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, 
        gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook)));
    vte_terminal_set_encoding(VTE_TERMINAL(tab.vte), (gchar*)data);
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
    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, 
        gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook)));
    vte_terminal_paste_clipboard(VTE_TERMINAL(tab.vte));
}

void termit_copy()
{
    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, 
        gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook)));
    vte_terminal_copy_clipboard(VTE_TERMINAL(tab.vte));
}

void termit_close_tab()
{
    termit_del_tab();
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 0)
        termit_quit();
}

static gboolean dlg_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    TRACE_NUM(event->keyval);
    switch (event->keyval)
    {
    case GDK_Return:
        g_signal_emit_by_name(GTK_OBJECT(widget), "response", GTK_RESPONSE_ACCEPT, NULL);
        TRACE_STR("Enter");
        break;
    case GDK_Escape:
        g_signal_emit_by_name(GTK_OBJECT(widget), "response", GTK_RESPONSE_REJECT, NULL);
        TRACE_STR("Escape");
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
    

    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, 
        gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook)));
    GtkWidget *label = gtk_label_new(_("Tab name"));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(
        GTK_ENTRY(entry), 
        gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(termit.notebook), tab.hbox));
    
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE, 5);
    
    g_signal_connect(G_OBJECT(dlg), "key-press-event", G_CALLBACK(dlg_key_press), dlg);
    

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), hbox, FALSE, FALSE, 10);
    gtk_widget_show_all(dlg);
    
    if (GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dlg)))
        gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(termit.notebook), tab.hbox, gtk_entry_get_text(GTK_ENTRY(entry)));
        
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
    TRACE_NUM(page_num);
    termit_set_statusbar_encoding(page_num);
}

void termit_cb_bookmarks_changed(GtkComboBox *widget, gpointer user_data)
{
    GtkTreeIter iter;
    if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter))
        return;

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gchar *value = NULL;
    gtk_tree_model_get(model, &iter, 1, &value, -1);

    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, 
        gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook)));
    gchar* cmd = g_strdup_printf("cd %s\n", value);
    GString* cmdStr = g_string_new(cmd);
    vte_terminal_feed_child(VTE_TERMINAL(tab.vte), cmdStr->str, cmdStr->len);
    
    g_string_free(cmdStr, TRUE);
    g_free(cmd);
    g_free(value);
    
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), tab.vte);
}

gint termit_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
    TRACE_NUM(event->button);
    if (event->type == GDK_2BUTTON_PRESS)
        termit_append_tab();

    return FALSE;
}

