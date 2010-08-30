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

#include <sys/wait.h>
#include <stdint.h>
#include <gdk/gdkkeysyms.h>

#include "termit.h"
#include "configs.h"
#include "sessions.h"
#include "termit_core_api.h"
#include "termit_style.h"
#include "lua_api.h"
#include "keybindings.h"
#include "callbacks.h"

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

gboolean termit_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return confirm_exit();
}

void termit_on_destroy(GtkWidget *widget, gpointer data)
{
    termit_quit();
}

void termit_on_tab_title_changed(VteTerminal *vte, gpointer user_data)
{
    if (!configs.allow_changing_title)
        return;
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    
    if (pTab->custom_tab_name)
        return;
    
    termit_tab_set_title(pTab, vte_terminal_get_window_title(VTE_TERMINAL(pTab->vte)));
}

void termit_on_toggle_scrollbar()
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

void termit_on_child_exited()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);

    TRACE("waiting for pid %d", pTab->pid);
    
    int status = 0;
    waitpid(pTab->pid, &status, WNOHANG);
    /* TODO: check wait return */    

    termit_close_tab();
}

static int termit_cursor_under_match(GdkEventButton* ev)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX2(pTab, page, -1);

    glong column = ((glong) (ev->x) / vte_terminal_get_char_width(VTE_TERMINAL(pTab->vte)));
    glong row = ((glong) (ev->y) / vte_terminal_get_char_height(VTE_TERMINAL(pTab->vte)));
    int tag = -1;
    char* current_match = vte_terminal_match_check(VTE_TERMINAL(pTab->vte), column, row, &tag);
    TRACE("column=%ld row=%ld current_match=%p tag=%d", column, row, current_match, tag);
    return tag;
}

static struct Match* get_match_by_tag(GArray* matches, int tag)
{
    if (tag < 0)
        return NULL;
    gint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        if (match->tag == tag)
            return match;
    }
    return NULL;
}

gboolean termit_on_popup(GtkWidget *widget, GdkEvent *event)
{
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    GdkEventButton *event_button = (GdkEventButton *) event;
    if (event_button->button == 3) {
        GtkMenu *menu = GTK_MENU(termit.menu);
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                          event_button->button, event_button->time);
        return TRUE;
    } else if (event_button->button == 1) {
        gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
        TERMIT_GET_TAB_BY_INDEX2(pTab, page, FALSE);
        
        int matchTag = termit_cursor_under_match(event_button);
        struct Match* match = get_match_by_tag(pTab->matches, matchTag);
        if (!match)
            return FALSE;
        TRACE("tag=%d match=[%s]", matchTag, match->pattern);
        termit_lua_dofunction(match->lua_callback);
    }

    return FALSE;
}

void termit_on_set_encoding(GtkWidget *widget, void *data)
{
    termit_set_encoding((const gchar*)data);
}

void termit_on_prev_tab()
{
    termit_prev_tab();
}

void termit_on_next_tab()
{
    termit_next_tab();
}

void termit_on_paste()
{
    termit_paste();
}

void termit_on_copy()
{
    termit_copy();
}

void termit_on_close_tab()
{
    termit_close_tab();
}

static gboolean dlg_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval) {
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

void termit_on_beep(VteTerminal *vte, gpointer user_data)
{
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    if (!pTab) {
        ERROR("pTab is NULL");
        return;
    }
    if (!gtk_window_has_toplevel_focus(GTK_WINDOW(termit.main_window))) {
        if (configs.urgency_on_bell) {
            gtk_window_set_urgency_hint(GTK_WINDOW(termit.main_window), TRUE);
            gchar* marked_title = g_strdup_printf("<b>%s</b>", gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
            gtk_label_set_markup(GTK_LABEL(pTab->tab_name), marked_title);
            g_free(marked_title);
        }
    }
}

gboolean termit_on_focus(GtkWidget *widget, GtkDirectionType arg1, gpointer user_data)
{
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    if (!pTab) {
        ERROR("pTab is NULL");
        return FALSE;
    }
    if (gtk_window_get_urgency_hint(GTK_WINDOW(termit.main_window))) {
        gtk_window_set_urgency_hint(GTK_WINDOW(termit.main_window), FALSE);
        gtk_label_set_markup(GTK_LABEL(pTab->tab_name), gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
        gtk_label_set_use_markup(GTK_LABEL(pTab->tab_name), FALSE);
    }
    return FALSE;
}

void termit_on_set_tab_name()
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
    
    if (GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dlg))) {
        termit_tab_set_title(pTab, gtk_entry_get_text(GTK_ENTRY(entry)));
        pTab->custom_tab_name = TRUE;
    }
    
    gtk_widget_destroy(dlg);
}

void termit_preferences_dialog(struct TermitTab *style);
void termit_on_edit_preferences()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    termit_preferences_dialog(pTab);
}
void termit_on_new_tab()
{
    termit_append_tab();
}

void termit_on_exit()
{
    if (confirm_exit() == FALSE)
        termit_quit();
}

void termit_on_switch_page(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data)
{
    TERMIT_GET_TAB_BY_INDEX(pTab, page_num);
    // it seems that set_active eventually calls toggle callback
    ((GtkCheckMenuItem*)termit.mi_show_scrollbar)->active = pTab->scrollbar_is_shown;
    termit_set_statusbar_encoding(page_num);
    if (configs.allow_changing_title)
        termit_set_window_title(pTab->title);
}

gint termit_on_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
    TRACE_MSG(__FUNCTION__);
    termit_mouse_event(event);
    return FALSE;
}

static gchar* termit_get_xdg_data_path()
{
    gchar* fullPath = NULL;
    const gchar *dataHome = g_getenv("XDG_DATA_HOME");
    if (dataHome)
        fullPath = g_strdup_printf("%s/termit", dataHome);
    else
        fullPath = g_strdup_printf("%s/.local/share/termit", g_getenv("HOME"));
    TRACE("XDG_DATA_PATH=%s", fullPath);
    return fullPath;
}

void termit_on_save_session()
{
/*  // debug   
    termit_save_session("tmpSess");
    return;
*/
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

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dlg);
        g_free(fullPath);
        return;
    }

    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    termit_save_session(filename);

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

void termit_on_user_menu_item_selected(GtkWidget *widget, void *data)
{
    struct UserMenuItem* pMi = (struct UserMenuItem*)data;
    TRACE("%s: (%d)", pMi->name, pMi->lua_callback);
    if (pMi->lua_callback) {
        termit_lua_dofunction(pMi->lua_callback);
    }
}

gboolean termit_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    return termit_key_event(event);
}

