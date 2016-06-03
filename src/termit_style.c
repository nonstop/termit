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

#include <config.h>

#include "termit_style.h"

void termit_style_init(struct TermitStyle* style)
{
    style->font_name = g_strdup("Monospace 10");
    style->font = pango_font_description_from_string(style->font_name);
    style->foreground_color = NULL;
    style->background_color = NULL;
    style->colors = NULL;
    style->colors_size = 0;
}

void termit_style_free(struct TermitStyle* style)
{
    g_free(style->font_name);
    pango_font_description_free(style->font);
    if (style->colors) {
        g_free(style->colors);
    }
    if (style->background_color) {
        gdk_rgba_free(style->background_color);
    }
    if (style->foreground_color) {
        gdk_rgba_free(style->foreground_color);
    }
    struct TermitStyle tmp = {};
    *style = tmp;
}

void termit_style_copy(struct TermitStyle* dest, const struct TermitStyle* src)
{
    dest->font_name = g_strdup(src->font_name);
    dest->font = pango_font_description_from_string(src->font_name);
    if (src->background_color) {
        dest->background_color = gdk_rgba_copy(src->background_color);
    } else {
        dest->background_color = NULL;
    }
    if (src->foreground_color) {
        dest->foreground_color = gdk_rgba_copy(src->foreground_color);
    } else {
        dest->foreground_color = NULL;
    }
    if (src->colors_size) {
        dest->colors = g_memdup(src->colors, src->colors_size * sizeof(GdkRGBA));
        dest->colors_size = src->colors_size;
    } else {
        dest->colors = NULL;
        dest->colors_size = 0;
    }
}
