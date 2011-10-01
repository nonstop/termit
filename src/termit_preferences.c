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
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_OK, NULL);
        break;
    case GDK_KEY_Escape:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_NONE, NULL);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void dlg_set_tab_color__(GtkColorButton *widget, gpointer user_data, void (*callback)(struct TermitTab*, const GdkColor*))
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    GdkColor color = {};
    gtk_color_button_get_color(widget, &color);
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
static void dlg_set_font(GtkFontButton *widget, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    termit_tab_set_font(pTab, gtk_font_button_get_font_name(widget));
}
static gboolean dlg_set_transparency(GtkSpinButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    gdouble value = gtk_spin_button_get_value(btn);
    termit_tab_set_transparency(pTab, value);
    return FALSE;
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
static gboolean dlg_set_visible_bell(GtkToggleButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    gboolean value = gtk_toggle_button_get_active(btn);
    termit_tab_set_visible_bell(pTab, value);
    return FALSE;
}

struct TermitDlgHelper
{
    struct TermitTab* pTab;
    gchar* tab_title;
    gboolean handmade_tab_title;
    struct TermitStyle style;
    gboolean au_bell;
    gboolean vi_bell;
    // widgets with values
    GtkWidget* dialog;
    GtkWidget* entry_title;
    GtkWidget* btn_font;
    GtkWidget* btn_foreground;
    GtkWidget* btn_background;
    GtkWidget* btn_image_file;
    GtkWidget* scale_transparency;
    GtkWidget* audible_bell;
    GtkWidget* visible_bell;
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
    hlp->vi_bell = pTab->visible_bell;
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
    if (hlp->tab_title)
        termit_tab_set_title(pTab, hlp->tab_title);
    vte_terminal_set_default_colors(VTE_TERMINAL(pTab->vte));
    termit_tab_set_font(pTab, hlp->style.font_name);
    termit_tab_set_background_image(pTab, hlp->style.image_file);
    termit_tab_set_color_foreground(pTab, hlp->style.foreground_color);
    termit_tab_set_color_background(pTab, hlp->style.background_color);
    termit_tab_set_transparency(pTab, hlp->style.transparency);
    termit_tab_set_audible_bell(pTab, hlp->au_bell);
    termit_tab_set_visible_bell(pTab, hlp->vi_bell);
    if (hlp->style.image_file) {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(hlp->btn_image_file), hlp->style.image_file);
    }
}

static void dlg_set_default_values(struct TermitDlgHelper* hlp)
{
    gtk_entry_set_text(GTK_ENTRY(hlp->entry_title), hlp->tab_title);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(hlp->btn_font), hlp->style.font_name);
    if (hlp->style.foreground_color) {
        gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_foreground), hlp->style.foreground_color);
    }
    if (hlp->style.background_color) {
        gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_background), hlp->style.background_color);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hlp->scale_transparency), hlp->style.transparency);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->audible_bell), hlp->au_bell);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->visible_bell), hlp->vi_bell);
    if (hlp->style.image_file) {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(hlp->btn_image_file), hlp->style.image_file);
    } else {
        gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(hlp->btn_image_file));
    }
}

static void dlg_restore_defaults(GtkButton *button, gpointer user_data)
{
    struct TermitDlgHelper* hlp = (struct TermitDlgHelper*)user_data;
    dlg_set_default_values(hlp);
    dlg_set_tab_default_values(hlp->pTab, hlp);
}

static void dlg_set_image_file(GtkFileChooserButton *widget, gpointer user_data)
{
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    termit_tab_set_background_image(pTab, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)));
}

static gboolean dlg_clear_image_file(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    if (event->keyval == GDK_KEY_Delete) {
        if (pTab->style.image_file) {
            gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(widget));
            termit_tab_set_background_image(pTab, NULL);
        }
    }
    return FALSE;
}

void termit_preferences_dialog(struct TermitTab *pTab)
{
    // store font_name, foreground, background
    struct TermitDlgHelper* hlp = termit_dlg_helper_new(pTab);

    GtkStockItem item = {};
    gtk_stock_lookup(GTK_STOCK_PREFERENCES, &item); // may be memory leak inside
    GtkWidget* dialog = gtk_dialog_new_with_buttons(item.label,
            GTK_WINDOW_TOPLEVEL,
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
            GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
            GTK_STOCK_OK, GTK_RESPONSE_OK,
            NULL);
    g_signal_connect(G_OBJECT(dialog), "key-press-event", G_CALLBACK(dlg_key_press), dialog);
    GtkWidget* dlg_table = gtk_table_new(5, 2, FALSE);

#define TERMIT_PREFERENCE_ROW(pref_name, widget) \
    gtk_table_attach(GTK_TABLE(dlg_table), gtk_label_new(pref_name), 0, 1, row, row + 1, 0, 0, 0, 0); \
    gtk_table_attach_defaults(GTK_TABLE(dlg_table), widget, 1, 2, row, row + 1); \
    hlp->widget = widget; \
    row++;
#define TERMIT_PREFERENCE_ROW2(pref_widget, widget) \
    gtk_table_attach(GTK_TABLE(dlg_table), pref_widget, 0, 1, row, row + 1, 0, 0, 0, 0); \
    gtk_table_attach_defaults(GTK_TABLE(dlg_table), widget, 1, 2, row, row + 1); \
    hlp->widget = widget; \
    row++;

    GtkWidget* entry_title = gtk_entry_new();
    guint row = 0;
    { // tab title
        gtk_entry_set_text(GTK_ENTRY(entry_title), hlp->tab_title);
        TERMIT_PREFERENCE_ROW(_("Title"), entry_title);
    }
    
    { // font selection
        GtkWidget* btn_font = gtk_font_button_new_with_font(pTab->style.font_name);
        g_signal_connect(btn_font, "font-set", G_CALLBACK(dlg_set_font), pTab);

        TERMIT_PREFERENCE_ROW(_("Font"), btn_font);
    }
    
    { // foreground
        GtkWidget* btn_foreground = (pTab->style.foreground_color)
            ? gtk_color_button_new_with_color(pTab->style.foreground_color)
            : gtk_color_button_new();
        g_signal_connect(btn_foreground, "color-set", G_CALLBACK(dlg_set_foreground), pTab);
    
        TERMIT_PREFERENCE_ROW(_("Foreground"), btn_foreground);
    }
    
    { // background
        GtkWidget* btn_background = (pTab->style.background_color)
            ? gtk_color_button_new_with_color(pTab->style.background_color)
            : gtk_color_button_new();
        g_signal_connect(btn_background, "color-set", G_CALLBACK(dlg_set_background), pTab);
        
        TERMIT_PREFERENCE_ROW(_("Background"), btn_background);
    }
    
    { // background image
        GtkWidget* btn_image_file = gtk_file_chooser_button_new(pTab->style.image_file,
            GTK_FILE_CHOOSER_ACTION_OPEN);
        GtkFileFilter* filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, _("images"));
        gtk_file_filter_add_mime_type(filter, "image/*");
        if (pTab->style.image_file) {
            gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(btn_image_file), pTab->style.image_file);
        }
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(btn_image_file), filter);
        g_signal_connect(btn_image_file, "file-set", G_CALLBACK(dlg_set_image_file), pTab);
        g_signal_connect(btn_image_file, "key-press-event", G_CALLBACK(dlg_clear_image_file), pTab);

        GtkWidget* btn_switch_image_file = gtk_check_button_new_with_label(_("Background image"));
        if (pTab->style.image_file) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_switch_image_file), TRUE);
        }
        /*g_signal_connect(btn_switch_image_file, "toggled", G_CALLBACK(dlg_switch_image_file), btn_image_file);*/

        /*TERMIT_PREFERENCE_ROW2(btn_switch_image_file, btn_image_file);*/
        TERMIT_PREFERENCE_ROW(_("Image"), btn_image_file);
    }

    { // transparency
        GtkWidget* scale_transparency = gtk_spin_button_new_with_range(0, 1, 0.05);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(scale_transparency), pTab->style.transparency);
        g_signal_connect(scale_transparency, "value-changed", G_CALLBACK(dlg_set_transparency), pTab);

        TERMIT_PREFERENCE_ROW(_("Transparency"), scale_transparency);
    }

    { // audible_bell
        GtkWidget* audible_bell = gtk_check_button_new();
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audible_bell), pTab->audible_bell);
        g_signal_connect(audible_bell, "toggled", G_CALLBACK(dlg_set_audible_bell), pTab);

        TERMIT_PREFERENCE_ROW(_("Audible bell"), audible_bell);
    }

    { // visible_bell
        GtkWidget* visible_bell = gtk_check_button_new();
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(visible_bell), pTab->visible_bell);
        g_signal_connect(visible_bell, "toggled", G_CALLBACK(dlg_set_visible_bell), pTab);

        TERMIT_PREFERENCE_ROW(_("Visible bell"), visible_bell);
    }

    {
        GtkWidget* btn_restore = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
        g_signal_connect(G_OBJECT(btn_restore), "clicked", G_CALLBACK(dlg_restore_defaults), hlp);
        gtk_table_attach(GTK_TABLE(dlg_table), btn_restore, 1, 2, row, row + 1, 0, 0, 0, 0);
    }
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), dlg_table);

    // TODO: apply to all tabs
    // TODO: color palette
    // TODO: save style - choose from saved (murphy, delek, etc.)
    
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
        dlg_set_tab_default_values(pTab, hlp);
    } else {
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
