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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include "termit.h"
#include "termit_style.h"
#include "termit_core_api.h"

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

static void dlg_set_tab_color__(GtkColorButton *widget, gpointer user_data, void (*callback)(struct TermitTab*, const GdkRGBA*))
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    GdkRGBA color = {};
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), &color);
    callback(pTab, &color);
}
static void dlg_set_foreground(GtkColorButton *widget, gpointer user_data)
{
    return dlg_set_tab_color__(widget, user_data, termit_tab_set_color_foreground);
}
static void dlg_set_background(GtkColorButton *widget, gpointer user_data)
{
    return dlg_set_tab_color__(widget, user_data, termit_tab_set_color_background);
}
static void dlg_set_font(GtkFontChooser *widget, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    termit_tab_set_font(pTab, gtk_font_chooser_get_font(widget));
}
static gboolean dlg_set_audible_bell(GtkToggleButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    gboolean value = gtk_toggle_button_get_active(btn);
    termit_tab_set_audible_bell(pTab, value);
    return FALSE;
}
static gboolean dlg_set_apply_to_all_tabs(GtkToggleButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    gboolean* flag = (gboolean*)user_data;
    *flag = gtk_toggle_button_get_active(btn);
    return FALSE;
}

struct TermitDlgHelper
{
    struct TermitTab* pTab;
    gchar* tab_title;
    gboolean handmade_tab_title;
    struct TermitStyle style;
    gboolean au_bell;
    // widgets with values
    GtkWidget* dialog;
    GtkWidget* entry_title;
    GtkWidget* btn_font;
    GtkWidget* btn_foreground;
    GtkWidget* btn_background;
    GtkWidget* btn_audible_bell;
    GtkWidget* btn_apply_to_all_tabs;
};

static struct TermitDlgHelper* termit_dlg_helper_new(struct TermitTab* pTab)
{
    struct TermitDlgHelper* hlp = g_malloc0(sizeof(struct TermitDlgHelper));
    hlp->pTab = pTab;
    if (pTab->title) {
        hlp->handmade_tab_title = TRUE;
        hlp->tab_title = g_strdup(pTab->title);
    } else {
        hlp->tab_title = g_strdup(gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
    }
    termit_style_copy(&hlp->style, &pTab->style);
    hlp->au_bell = pTab->audible_bell;
    return hlp;
}

static void termit_dlg_helper_free(struct TermitDlgHelper* hlp)
{
    g_free(hlp->tab_title);
    termit_style_free(&hlp->style);
    g_free(hlp);
}

static void dlg_set_tab_default_values(struct TermitTab* pTab, struct TermitDlgHelper* hlp)
{
    if (hlp->tab_title) {
        termit_tab_set_title(pTab, hlp->tab_title);
    }
    termit_style_free(&pTab->style);
    termit_style_copy(&pTab->style, &hlp->style);
    termit_tab_apply_colors(pTab);
    termit_tab_set_font(pTab, hlp->style.font_name);
    termit_tab_set_audible_bell(pTab, hlp->au_bell);
}

static void dlg_set_default_values(struct TermitDlgHelper* hlp)
{
    gtk_entry_set_text(GTK_ENTRY(hlp->entry_title), hlp->tab_title);
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(hlp->btn_font), hlp->style.font_name);
    if (hlp->style.foreground_color) {
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(hlp->btn_foreground), hlp->style.foreground_color);
    }
    if (hlp->style.background_color) {
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(hlp->btn_background), hlp->style.background_color);
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->btn_audible_bell), hlp->au_bell);
}

static void dlg_restore_defaults(GtkButton *button, gpointer user_data)
{
    struct TermitDlgHelper* hlp = (struct TermitDlgHelper*)user_data;
    dlg_set_default_values(hlp);
    dlg_set_tab_default_values(hlp->pTab, hlp);
}

void termit_preferences_dialog(struct TermitTab *pTab)
{
    // store font_name, foreground, background
    struct TermitDlgHelper* hlp = termit_dlg_helper_new(pTab);

    GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Preferences"),
            GTK_WINDOW(termit.main_window),
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
            "_Cancel", GTK_RESPONSE_REJECT,
            "_Apply", GTK_RESPONSE_ACCEPT,
            NULL);
    g_signal_connect(G_OBJECT(dialog), "key-press-event", G_CALLBACK(dlg_key_press), dialog);
    GtkWidget* grid = gtk_grid_new();

#define TERMIT_PREFERENCE_ROW(pref_name, widget) \
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new(pref_name), 0, row, 1, 1); \
    gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1); \
    hlp->widget = widget; \
    row++;

    gboolean apply_to_all_tabs_flag = FALSE;
    GtkWidget* entry_title = gtk_entry_new();
    guint row = 0;
    { // tab title
        gtk_entry_set_text(GTK_ENTRY(entry_title), hlp->tab_title);
        TERMIT_PREFERENCE_ROW(_("Title"), entry_title);
    }

    // font selection
    GtkWidget* btn_font = gtk_font_button_new_with_font(pTab->style.font_name);
    g_signal_connect(btn_font, "font-set", G_CALLBACK(dlg_set_font), pTab);
    TERMIT_PREFERENCE_ROW(_("Font"), btn_font);

    // foreground
    GtkWidget* btn_foreground = (pTab->style.foreground_color)
        ? gtk_color_button_new_with_rgba(pTab->style.foreground_color)
        : gtk_color_button_new();
    g_signal_connect(btn_foreground, "color-set", G_CALLBACK(dlg_set_foreground), pTab);
    TERMIT_PREFERENCE_ROW(_("Foreground"), btn_foreground);

    // background
    GtkWidget* btn_background = (pTab->style.background_color)
        ? gtk_color_button_new_with_rgba(pTab->style.background_color)
        : gtk_color_button_new();
    g_signal_connect(btn_background, "color-set", G_CALLBACK(dlg_set_background), pTab);
    TERMIT_PREFERENCE_ROW(_("Background"), btn_background);

    // audible_bell
    GtkWidget* btn_audible_bell = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_audible_bell), pTab->audible_bell);
    g_signal_connect(btn_audible_bell, "toggled", G_CALLBACK(dlg_set_audible_bell), pTab);
    TERMIT_PREFERENCE_ROW(_("audible bell"), btn_audible_bell);

    // apply to al tabs
    GtkWidget* btn_apply_to_all_tabs = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_apply_to_all_tabs), FALSE);
    g_signal_connect(btn_apply_to_all_tabs, "toggled", G_CALLBACK(dlg_set_apply_to_all_tabs), &apply_to_all_tabs_flag);
    TERMIT_PREFERENCE_ROW(_("Apply to all tabs"), btn_apply_to_all_tabs);

    GtkWidget* btn_restore = gtk_button_new_from_icon_name("document-revert", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(G_OBJECT(btn_restore), "clicked", G_CALLBACK(dlg_restore_defaults), hlp);
    gtk_grid_attach(GTK_GRID(grid), btn_restore, 0, row, 2, 1);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        dlg_set_tab_default_values(pTab, hlp);
    } else {
        if (apply_to_all_tabs_flag) {
            gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
            gint i=0;
            for (; i<page_num; ++i) {
                TERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
                dlg_set_font(GTK_FONT_CHOOSER(btn_font), pTab);
                dlg_set_foreground(GTK_COLOR_BUTTON(btn_foreground), pTab);
                dlg_set_background(GTK_COLOR_BUTTON(btn_background), pTab);
                dlg_set_audible_bell(GTK_TOGGLE_BUTTON(btn_audible_bell), pTab);
            }
        }
        // insane title flag
        if (pTab->title ||
                (!pTab->title &&
                 strcmp(gtk_label_get_text(GTK_LABEL(pTab->tab_name)),
                     gtk_entry_get_text(GTK_ENTRY(entry_title))) != 0)) {
            termit_tab_set_title(pTab, gtk_entry_get_text(GTK_ENTRY(entry_title)));
        }
    }
    termit_dlg_helper_free(hlp);
    gtk_widget_destroy(dialog);
}
