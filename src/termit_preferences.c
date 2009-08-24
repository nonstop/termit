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
    case GDK_Return:
        g_signal_emit_by_name(GTK_OBJECT(widget), "response", GTK_RESPONSE_OK, NULL);
        break;
    case GDK_Escape:
        g_signal_emit_by_name(GTK_OBJECT(widget), "response", GTK_RESPONSE_NONE, NULL);
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
    GdkColor color = {0};
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
    gdouble transparency;
    gchar* font_name;
    GdkColor foreground_color;
    GdkColor background_color;
    gboolean au_bell;
    gboolean vi_bell;
    // widgets with values
    GtkWidget* dialog;
    GtkWidget* entry_title;
    GtkWidget* btn_font;
    GtkWidget* btn_foreground;
    GtkWidget* btn_background;
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
    hlp->font_name = g_strdup(pTab->style.font_name);
    hlp->foreground_color = pTab->style.foreground_color;
    hlp->background_color = pTab->style.background_color;
    hlp->transparency = pTab->style.transparency;
    hlp->au_bell = pTab->audible_bell;
    hlp->vi_bell = pTab->visible_bell;
    return hlp;
}

static void termit_dlg_helper_free(struct TermitDlgHelper* hlp)
{
    g_free(hlp->tab_title);
    g_free(hlp->font_name);
    g_free(hlp);
}

static void dlg_set_tab_default_values(struct TermitTab* pTab, struct TermitDlgHelper* hlp)
{
    if (hlp->tab_title)
        termit_tab_set_title(pTab, hlp->tab_title);
    termit_tab_set_font(pTab, hlp->font_name);
    termit_tab_set_color_foreground(pTab, &hlp->foreground_color);
    termit_tab_set_color_background(pTab, &hlp->background_color);
    termit_tab_set_transparency(pTab, hlp->transparency);
    termit_tab_set_audible_bell(pTab, hlp->au_bell);
    termit_tab_set_visible_bell(pTab, hlp->vi_bell);
}

static void dlg_set_default_values(struct TermitDlgHelper* hlp)
{
    gtk_entry_set_text(GTK_ENTRY(hlp->entry_title), hlp->tab_title);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(hlp->btn_font), hlp->font_name);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_foreground), &hlp->foreground_color);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_background), &hlp->background_color);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hlp->scale_transparency), hlp->transparency);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->audible_bell), hlp->au_bell);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->visible_bell), hlp->vi_bell);
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

    GtkStockItem item = {0};
    gtk_stock_lookup(GTK_STOCK_PREFERENCES, &item); // may be memory leak inside
    GtkWidget* dialog = gtk_dialog_new_with_buttons(item.label,
            GTK_WINDOW_TOPLEVEL,
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
            GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
            GTK_STOCK_OK, GTK_RESPONSE_OK,
            NULL);
    gtk_dialog_set_has_separator(GTK_DIALOG(dialog), TRUE);
    g_signal_connect(G_OBJECT(dialog), "key-press-event", G_CALLBACK(dlg_key_press), dialog);
    GtkWidget* dlg_table = gtk_table_new(5, 2, FALSE);

#define TERMIT_PREFERENCE_ROW(pref_name, widget) \
    gtk_table_attach(GTK_TABLE(dlg_table), gtk_label_new(pref_name), 0, 1, row, row + 1, 0, 0, 0, 0); \
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
        GtkWidget* btn_foreground = gtk_color_button_new_with_color(&pTab->style.foreground_color);
        g_signal_connect(btn_foreground, "color-set", G_CALLBACK(dlg_set_foreground), pTab);
    
        TERMIT_PREFERENCE_ROW(_("Foreground"), btn_foreground);
    }
    
    { // background
        GtkWidget* btn_background = gtk_color_button_new_with_color(&pTab->style.background_color);
        g_signal_connect(btn_background, "color-set", G_CALLBACK(dlg_set_background), pTab);
        
        TERMIT_PREFERENCE_ROW(_("Background"), btn_background);
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
