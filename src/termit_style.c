#include <string.h>
#include <gtk/gtk.h>

#include <config.h>

#include "termit_style.h"

void termit_style_init(struct TermitStyle* style)
{
    style->font_name = g_strdup("Monospace 10");
    style->font = pango_font_description_from_string(style->font_name);
    {
        GdkColor color = {0};
        if (gdk_color_parse("gray", &color) == TRUE) {
            style->foreground_color = color;
        }
    }
    {
        GdkColor color = {0};
        if (gdk_color_parse("black", &color) == TRUE) {
            style->background_color = color;
        }
    }
    style->transparency = 0;    
}

void termit_style_free(struct TermitStyle* style)
{
    g_free(style->font_name);
    pango_font_description_free(style->font);
    struct TermitStyle tmp = {0};
    *style = tmp;
}

void termit_style_copy(struct TermitStyle* dest, const struct TermitStyle* src)
{
    dest->font_name = g_strdup(src->font_name);
    dest->foreground_color = src->foreground_color;
    dest->background_color = src->background_color;
    dest->font = pango_font_description_from_string(src->font_name);
    dest->transparency = src->transparency;
}

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

gint termit_style_dialog (struct TermitStyle *style)
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
