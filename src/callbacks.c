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

#include <sys/wait.h>
#include <stdint.h>
#include <string.h>
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
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) <= 1) {
        return FALSE;
    }

    GtkWidget *dlg = gtk_message_dialog_new(
        GTK_WINDOW(termit.main_window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO, _("Several tabs are opened.\nClose anyway?"));
    gint response = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    return (response == GTK_RESPONSE_YES) ? FALSE : TRUE;
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
    if (!configs.allow_changing_title) {
        return;
    }
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);

    if (pTab->custom_tab_name) {
        return;
    }

    termit_tab_set_title(pTab, vte_terminal_get_window_title(VTE_TERMINAL(pTab->vte)));
}

gboolean termit_on_search_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval) {
    case GDK_KEY_Return: {
        if (event->state == GDK_CONTROL_MASK) {
            termit_on_find_prev(NULL, NULL);
        } else {
            termit_on_find_next(NULL, NULL);
        }
        return TRUE;
    }
    case GDK_KEY_Escape: {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(termit.b_toggle_search), FALSE);
        return TRUE;
    }
    default:
        return FALSE;
    }
    return FALSE;
}

static void termit_search_prepare_regex(const gchar* searchRegex)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    if (strlen(searchRegex) == 0) {
        VteRegex* regex = vte_terminal_search_get_regex(VTE_TERMINAL(pTab->vte));
        if (regex) {
            vte_regex_unref(regex);
        }
        vte_terminal_search_set_regex(VTE_TERMINAL(pTab->vte), NULL, 0);
    } else {
        VteRegex* currSearchRegex = vte_terminal_search_get_regex(VTE_TERMINAL(pTab->vte));
        if (currSearchRegex && strcmp(pTab->search_regex, searchRegex) == 0) {
            return;
        }
        if (currSearchRegex) {
            vte_regex_unref(currSearchRegex);
        }
        if (pTab->search_regex) {
            g_free(pTab->search_regex);
        }
        GError* err = NULL;
        VteRegex* regex = vte_regex_new_for_search(searchRegex, -1, VTE_REGEX_FLAGS_DEFAULT, &err);
        if (err) {
            TRACE("failed to compile regex [%s]: skipping", searchRegex);
            return;
        }
        vte_terminal_search_set_regex(VTE_TERMINAL(pTab->vte), regex, 0);
        pTab->search_regex = g_strdup(searchRegex);
    }
}

void termit_on_find_next(GtkButton* btn, gpointer user_data)
{
    termit_search_prepare_regex(gtk_entry_get_text(GTK_ENTRY(termit.search_entry)));
    termit_search_find_next();
}

void termit_on_find_prev(GtkButton* btn, gpointer user_data)
{
    termit_search_prepare_regex(gtk_entry_get_text(GTK_ENTRY(termit.search_entry)));
    termit_search_find_prev();
}

void termit_on_toggle_search(GtkToggleButton* tb, gpointer user_data)
{
    termit_toggle_search();
}

void termit_on_toggle_scrollbar()
{
    TRACE_MSG(__FUNCTION__);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);

    if (pTab->scrollbar_is_shown) {
        gtk_widget_hide(GTK_WIDGET(pTab->scrollbar));
    } else {
        gtk_widget_show(GTK_WIDGET(pTab->scrollbar));
    }
    pTab->scrollbar_is_shown = !pTab->scrollbar_is_shown;
}

void termit_on_child_exited(VteTerminal* vte, gint status, gpointer user_data)
{
    gint page = -1;
    struct TermitTab* pTab = termit_get_tab_by_vte(vte, &page);
    if (!pTab) {
        return;
    }
    TRACE("child process exited with status %d", status);
    termit_del_tab_n(page);
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 0) {
        TRACE("no pages left, exiting");
        termit_quit();
    }
}

static int termit_cursor_under_match(GdkEvent* ev, char** matchedText)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return -1);
    int tag = -1;
    *matchedText = vte_terminal_match_check_event(VTE_TERMINAL(pTab->vte), ev, &tag);
    return tag;
}

static struct Match* get_match_by_tag(GArray* matches, int tag)
{
    guint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        if (match->tag == tag) {
            return match;
        }
    }
    return NULL;
}

gboolean termit_on_popup(GtkWidget *widget, GdkEvent *event)
{
    if (event->type != GDK_BUTTON_PRESS) {
        return FALSE;
    }

    GdkEventButton *event_button = (GdkEventButton *) event;
    if (event_button->button == 3) {
        GtkMenu *menu = GTK_MENU(termit.menu);
        gtk_menu_popup_at_pointer(menu, NULL);
        return TRUE;
    } else if (event_button->button == 1) {
        gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
        TERMIT_GET_TAB_BY_INDEX(pTab, page, return FALSE);
        char* matchedText = NULL;
        int matchTag = termit_cursor_under_match(event, &matchedText);
        if (!matchedText) {
            return FALSE;
        }
        struct Match* match = get_match_by_tag(pTab->matches, matchTag);
        if (!match) {
            g_free(matchedText);
            return FALSE;
        }
        TRACE("tag=%d match=[%s]", matchTag, match->pattern);
        termit_lua_domatch(match->lua_callback, matchedText);
        g_free(matchedText);
    }

    return FALSE;
}

static gboolean dlg_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval) {
    case GDK_KEY_Return:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_ACCEPT, NULL);
        break;
    case GDK_KEY_Escape:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_REJECT, NULL);
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
        "_Cancel", GTK_RESPONSE_REJECT,
        "_OK", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);

    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    GtkWidget *label = gtk_label_new(_("Tab name"));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(
        GTK_ENTRY(entry),
        gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(termit.notebook), pTab->hbox));

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(dlg), "key-press-event", G_CALLBACK(dlg_key_press), dlg);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), hbox, FALSE, FALSE, 10);
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
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    termit_preferences_dialog(pTab);
}

void termit_on_exit()
{
    if (confirm_exit() == FALSE) {
        termit_quit();
    }
}

void termit_on_switch_page(GtkNotebook *notebook, gpointer arg, guint page, gpointer user_data)
{
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s page=%d vte=%p", __FUNCTION__, page, pTab->vte);
    // it seems that set_active eventually calls toggle callback
    /*((GtkCheckMenuItem*)termit.mi_show_scrollbar)->active = pTab->scrollbar_is_shown;*/
    gpointer pHandlerId = g_object_get_data(G_OBJECT(termit.mi_show_scrollbar), "handlerId");
    if (pHandlerId) {
        g_signal_handler_disconnect(G_OBJECT(termit.mi_show_scrollbar), *((gulong*)pHandlerId));
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(termit.mi_show_scrollbar), pTab->scrollbar_is_shown);
    termit_set_show_scrollbar_signal(termit.mi_show_scrollbar, pHandlerId);

    termit_set_statusbar_message(page);
    if (configs.allow_changing_title) {
        termit_set_window_title(pTab->title);
    }
}

gint termit_on_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
    termit_mouse_event(event);
    return FALSE;
}

static gchar* termit_get_xdg_data_path()
{
    gchar* fullPath = NULL;
    const gchar *dataHome = g_getenv("XDG_DATA_HOME");
    if (dataHome) {
        fullPath = g_strdup_printf("%s/termit", dataHome);
    } else {
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
        "_Cancel", GTK_RESPONSE_REJECT,
        "_Save", GTK_RESPONSE_ACCEPT,
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
        "_Cancel", GTK_RESPONSE_REJECT,
        "_OK", GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), fullPath);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT) {
        goto free_dlg;
    }

    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    termit_load_session(filename);
    g_free(filename);
free_dlg:
    gtk_widget_destroy(dlg);
    g_free(fullPath);
}

void termit_on_menu_item_selected(GtkWidget *widget, void *data)
{
    struct UserMenuItem* pMi = (struct UserMenuItem*)g_object_get_data(G_OBJECT(widget),
            TERMIT_USER_MENU_ITEM_DATA);
    if (pMi && pMi->lua_callback) {
        termit_lua_dofunction(pMi->lua_callback);
    }
}

gboolean termit_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    return termit_key_event(event);
}
