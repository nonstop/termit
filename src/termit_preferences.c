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

void termit_preferences_dialog(struct TermitTab *pTab)
{
    // store font_name, foreground, background
    gchar* tab_title = (pTab->title) ? g_strdup(pTab->title) : NULL;
    gchar* font_name = g_strdup(pTab->style.font_name);
    GdkColor foreground_color = pTab->style.foreground_color;
    GdkColor background_color = pTab->style.background_color;

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
    GtkWidget* dlg_content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    
    GtkWidget* entry_title = gtk_entry_new();
    { // tab title
        GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
        gtk_entry_set_text(GTK_ENTRY(entry_title),
                (pTab->title) ? tab_title : gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
        gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new(_("Title")));
        gtk_container_add(GTK_CONTAINER(hbox), entry_title);
        gtk_container_add(GTK_CONTAINER(dlg_content), hbox);
    }
    
    { // font selection
        GtkWidget* btn_font = gtk_font_button_new_with_font(pTab->style.font_name);
        g_signal_connect(btn_font, "font-set", G_CALLBACK(dlg_set_font), pTab);

        GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new(_("Font")));
        gtk_container_add(GTK_CONTAINER(hbox), btn_font);
        gtk_container_add(GTK_CONTAINER(dlg_content), hbox);
    }
    
    { // foreground
        GtkWidget* btn_foreground = gtk_color_button_new_with_color(&pTab->style.foreground_color);
        g_signal_connect(btn_foreground, "color-set", G_CALLBACK(dlg_set_foreground), pTab);

        GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new(_("Foreground")));
        gtk_container_add(GTK_CONTAINER(hbox), btn_foreground);
        gtk_container_add(GTK_CONTAINER(dlg_content), hbox);
    }
    
    { // background
        GtkWidget* btn_background = gtk_color_button_new_with_color(&pTab->style.background_color);
        g_signal_connect(btn_background, "color-set", G_CALLBACK(dlg_set_background), pTab);
        
        GtkWidget* hbox = gtk_hbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new(_("Background")));
        gtk_container_add(GTK_CONTAINER(hbox), btn_background);
        gtk_container_add(GTK_CONTAINER(dlg_content), hbox);
    }
    // TODO: restore default
    // TODO: apply to all tabs
    // TODO: check grid layout - table
    // TODO: alpha
    // TODO: audible_bell
    // TODO: visible_bell
    // TODO: color palette
    // TODO: save style - choose from saved (murphy, delek, etc.)
    
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
        if (pTab->title)
            termit_set_tab_title(pTab, tab_title);
        termit_set_tab_font(pTab, font_name);
        termit_set_tab_color_foreground(pTab, &foreground_color);
        termit_set_tab_color_background(pTab, &background_color);
    } else {
        // insane title flag
        if (pTab->title ||
                (!pTab->title &&
                 strcmp(gtk_label_get_text(GTK_LABEL(pTab->tab_name)),
                     gtk_entry_get_text(GTK_ENTRY(entry_title))) != 0)) {
            termit_set_tab_title(pTab, gtk_entry_get_text(GTK_ENTRY(entry_title)));
        }
    }
    g_free(tab_title);
    g_free(font_name);
    gtk_widget_destroy(dialog);
}
