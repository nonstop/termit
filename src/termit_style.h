#ifndef TERMIT_STYLE_H
#define TERMIT_STYLE_H

#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

struct TermitStyle
{
    gchar* font_name;
    PangoFontDescription* font;
    GdkColor foreground_color;
    GdkColor background_color;
    gdouble transparency;
};

void termit_style_init(struct TermitStyle* style);
void termit_style_copy(struct TermitStyle* dest, const struct TermitStyle* src);
void termit_style_free(struct TermitStyle* style);

#endif /* TERMIT_STYLE_H */

