#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include "termit.h"
#include "termit_style.h"
#include "termit_core_api.h"

#if 0
struct {
  GtkWidget *fgcolorsel;
  GtkWidget *bgcolorsel;
  GtkWidget *preview;
} my_style;
  
//For some reason gtk_widget_set_style doesn't force an update when changing colors
//And gtk_widget_modify_* doesn't play nice with gtk_widget_set_style
//So we use 2 seperate callbacks.
static void font_changed (GtkWidget *entry)
{
    GtkStyle *style;
    style = gtk_widget_get_style(entry);
    gtk_widget_modify_font(my_style.preview, style->font_desc);
}

static void color_changed (GtkWidget *entry)
{
    GdkColor fg_color, bg_color;
    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION (my_style.fgcolorsel), &fg_color);
    gtk_color_selection_get_current_color(GTK_COLOR_SELECTION (my_style.bgcolorsel), &bg_color);
    gtk_widget_modify_base(my_style.preview, 0, &bg_color);
    gtk_widget_modify_text(my_style.preview, 0, &fg_color);
}

//This routine hides the preview box from the font_selection widget so we can display a common one.
void hack_fontsel(GtkWidget *fontsel)
{
    GList *children;
    GtkBoxChild *child;
    GtkWidget *gp_vbox, *preview;

    //This is dependant on gtk implementation and is very bad!
    //Get preview box
#if GTK_MINOR_VERSION >= 14
    preview = gtk_font_selection_get_preview_entry(GTK_FONT_SELECTION (fontsel) );
#else
    preview = GTK_FONT_SELECTION (fontsel)->preview_entry;
#endif

    g_signal_connect (preview, "style-set",
                      G_CALLBACK (font_changed), NULL);
    //Get grand-parent vbox
    gp_vbox = gtk_widget_get_parent(gtk_widget_get_parent(preview));
    children = g_list_last(GTK_BOX (gp_vbox)->children);
    child = children->data;
    gtk_widget_hide(child->widget);
    children = g_list_previous(children);
    child = children->data;
    gtk_widget_hide(child->widget);
}
gint termit_preferences_dialog (struct TermitStyle *style)
{
    GtkWidget *dialog, *fontsel;
    GtkWidget *notebook;
    gint result;
 
    /* Create the widgets */
  
    dialog = gtk_dialog_new_with_buttons ("Message",
                                          GTK_WINDOW_TOPLEVEL,
                                          GTK_DIALOG_DESTROY_WITH_PARENT |
                                          GTK_DIALOG_MODAL,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_NONE,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          NULL);
     
    my_style.preview = gtk_entry_new();
    gtk_entry_set_text (GTK_ENTRY (my_style.preview), "abcdefghijk ABCDEFGHIJK");
    notebook = gtk_notebook_new();

    fontsel = gtk_font_selection_new ();
    hack_fontsel(fontsel);
    gtk_widget_show(fontsel);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), fontsel, gtk_label_new("Font"));

    my_style.fgcolorsel = gtk_color_selection_new();
    g_signal_connect (my_style.fgcolorsel, "color_changed",
                      G_CALLBACK (color_changed), NULL);
    gtk_widget_show(my_style.fgcolorsel);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), my_style.fgcolorsel, gtk_label_new("Foreground"));

    my_style.bgcolorsel = gtk_color_selection_new();
    g_signal_connect (my_style.bgcolorsel, "color_changed",
                      G_CALLBACK (color_changed), NULL);
    gtk_widget_show(my_style.bgcolorsel);
    gtk_notebook_append_page(GTK_NOTEBOOK (notebook), my_style.bgcolorsel, gtk_label_new("Background"));

    gtk_widget_show(notebook);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                        notebook, TRUE, TRUE, 0);
    gtk_widget_show(my_style.preview);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                        my_style.preview,  TRUE, TRUE, 0);
    /* and the window */
    gtk_font_selection_set_font_name(GTK_FONT_SELECTION(fontsel), style->font_name);
    gtk_color_selection_set_current_color(GTK_COLOR_SELECTION (my_style.fgcolorsel),
                                          &style->foreground_color);
    gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION (my_style.fgcolorsel), FALSE);
    gtk_color_selection_set_current_color(GTK_COLOR_SELECTION (my_style.bgcolorsel),
                                          &style->background_color);
    gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION (my_style.fgcolorsel), TRUE);
    result = gtk_dialog_run (GTK_DIALOG (dialog));
    if(result == GTK_RESPONSE_OK) {
        termit_style_free(style);
        style->font_name = g_strdup(gtk_font_selection_get_font_name(GTK_FONT_SELECTION (fontsel)));
        gtk_color_selection_get_current_color(GTK_COLOR_SELECTION (my_style.fgcolorsel), &style->foreground_color);
        gtk_color_selection_get_current_color(GTK_COLOR_SELECTION (my_style.bgcolorsel), &style->background_color);
       /*style->alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION (my_style.bgcolorsel));*/
    }
    gtk_widget_destroy (dialog);
    return (result);
}
#endif
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
    return dlg_set_tab_color__(widget, user_data, termit_set_tab_color_foreground);
}
static void dlg_set_background(GtkColorButton *widget, gpointer user_data)
{
    return dlg_set_tab_color__(widget, user_data, termit_set_tab_color_background);
}
static void dlg_set_font(GtkFontButton *widget, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    termit_set_tab_font(pTab, gtk_font_button_get_font_name(widget));
}
static gboolean dlg_set_transparency(GtkRange *range, GtkScrollType scrolltype, gdouble value, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct TermitTab* pTab = (struct TermitTab*)user_data;
    termit_set_tab_transparency(pTab, value);
    return FALSE;
}

struct TermitDlgHelper
{
    gchar* tab_title;
    gboolean handmade_tab_title;
    gdouble transparency;
    gchar* font_name;
    GdkColor foreground_color;
    GdkColor background_color;
    // widgets with values
    GtkWidget* dialog;
    GtkWidget* entry_title;
    GtkWidget* btn_font;
    GtkWidget* btn_foreground;
    GtkWidget* btn_background;
    GtkWidget* btn_transparency;
};

static struct TermitDlgHelper* termit_dlg_helper_new(struct TermitTab* pTab)
{
    struct TermitDlgHelper* hlp = g_malloc0(sizeof(struct TermitDlgHelper));
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
    return hlp;
}

static void termit_dlg_helper_free(struct TermitDlgHelper* hlp)
{
    g_free(hlp->tab_title);
    g_free(hlp->font_name);
    g_free(hlp);
}

static void dlg_set_default_values(struct TermitDlgHelper* hlp)
{
    gtk_entry_set_text(GTK_ENTRY(hlp->entry_title), hlp->tab_title);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(hlp->btn_font), hlp->font_name);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_foreground), &hlp->foreground_color);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_background), &hlp->background_color);
    gtk_range_set_value(GTK_RANGE(hlp->btn_transparency), hlp->transparency);
}

static void dlg_restore_defaults(GtkButton *button, gpointer user_data)
{
    struct TermitDlgHelper* hlp = (struct TermitDlgHelper*)user_data;
    dlg_set_default_values(hlp);
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
    gtk_table_attach(GTK_TABLE(dlg_table), gtk_label_new(_(pref_name)), 0, 1, row, row + 1, 0, 0, 0, 0); \
    gtk_table_attach_defaults(GTK_TABLE(dlg_table), widget, 1, 2, row, row + 1); \
    row++;

    GtkWidget* entry_title = gtk_entry_new();
    guint row = 0;
    { // tab title
        gtk_entry_set_text(GTK_ENTRY(entry_title), hlp->tab_title);
        hlp->entry_title = entry_title;
        TERMIT_PREFERENCE_ROW("Title", entry_title);
    }
    
    { // font selection
        GtkWidget* btn_font = gtk_font_button_new_with_font(pTab->style.font_name);
        g_signal_connect(btn_font, "font-set", G_CALLBACK(dlg_set_font), pTab);

        TERMIT_PREFERENCE_ROW("Font", btn_font);
        hlp->btn_font = btn_font;
    }
    
    { // foreground
        GtkWidget* btn_foreground = gtk_color_button_new_with_color(&pTab->style.foreground_color);
        g_signal_connect(btn_foreground, "color-set", G_CALLBACK(dlg_set_foreground), pTab);
    
        TERMIT_PREFERENCE_ROW("Foreground", btn_foreground);
        hlp->btn_foreground = btn_foreground;
    }
    
    { // background
        GtkWidget* btn_background = gtk_color_button_new_with_color(&pTab->style.background_color);
        g_signal_connect(btn_background, "color-set", G_CALLBACK(dlg_set_background), pTab);
        
        TERMIT_PREFERENCE_ROW("Background", btn_background);
        hlp->btn_background = btn_background;
    }
    
    { // transparency
        GtkWidget* btn_transparency = gtk_hscale_new_with_range(0, 1, 0.05);
        gtk_range_set_value(GTK_RANGE(btn_transparency), pTab->style.transparency);
        g_signal_connect(btn_transparency, "change-value", G_CALLBACK(dlg_set_transparency), pTab);

        TERMIT_PREFERENCE_ROW("Background", btn_transparency);
        hlp->btn_transparency = btn_transparency;
    }

    {
        GtkWidget* btn_restore = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
        g_signal_connect(G_OBJECT(btn_restore), "clicked", G_CALLBACK(dlg_restore_defaults), hlp);
        gtk_table_attach(GTK_TABLE(dlg_table), btn_restore, 1, 2, row, row + 1, 0, 0, 0, 0);
    }
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), dlg_table);

    // TODO: apply to all tabs
    // TODO: audible_bell
    // TODO: visible_bell
    // TODO: color palette
    // TODO: save style - choose from saved (murphy, delek, etc.)
    
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
        if (hlp->tab_title)
            termit_set_tab_title(pTab, hlp->tab_title);
        termit_set_tab_font(pTab, hlp->font_name);
        termit_set_tab_color_foreground(pTab, &hlp->foreground_color);
        termit_set_tab_color_background(pTab, &hlp->background_color);
        termit_set_tab_transparency(pTab, hlp->transparency);
    } else {
        // insane title flag
        if (pTab->title ||
                (!pTab->title &&
                 strcmp(gtk_label_get_text(GTK_LABEL(pTab->tab_name)),
                     gtk_entry_get_text(GTK_ENTRY(entry_title))) != 0)) {
            termit_set_tab_title(pTab, gtk_entry_get_text(GTK_ENTRY(entry_title)));
        }
    }
    termit_dlg_helper_free(hlp);
    gtk_widget_destroy(dialog);
}
