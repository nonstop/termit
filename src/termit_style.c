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

#include <config.h>

#include "termit_style.h"

void termit_style_init(struct TermitStyle* style)
{
    style->font_name = g_strdup("Monospace 10");
    style->font = pango_font_description_from_string(style->font_name);
    style->foreground_color = NULL;
    style->background_color = NULL;
    style->colors = NULL;
    style->image_file = NULL;
    style->colors_size = 0;
    style->transparency = 0.0;
    style->opacity = 0.0;
}

void termit_style_free(struct TermitStyle* style)
{
    g_free(style->font_name);
    pango_font_description_free(style->font);
    if (style->colors) {
        g_free(style->colors);
    }
    if (style->image_file) {
        g_free(style->image_file);
    }
    if (style->background_color) {
        gdk_color_free(style->background_color);
    }
    if (style->foreground_color) {
        gdk_color_free(style->foreground_color);
    }
    struct TermitStyle tmp = {};
    *style = tmp;
}

void termit_style_copy(struct TermitStyle* dest, const struct TermitStyle* src)
{
    dest->font_name = g_strdup(src->font_name);
    dest->font = pango_font_description_from_string(src->font_name);
    if (src->background_color) {
        dest->background_color = gdk_color_copy(src->background_color);
    } else {
        dest->background_color = NULL;
    }
    if (src->foreground_color) {
        dest->foreground_color = gdk_color_copy(src->foreground_color);
    } else {
        dest->foreground_color = NULL;
    }
    dest->transparency = src->transparency;
    dest->opacity = src->opacity;
    dest->image_file = g_strdup(src->image_file);
    if (src->colors_size) {
        dest->colors = g_memdup(src->colors, src->colors_size * sizeof(GdkColor));
        dest->colors_size = src->colors_size;
    } else {
        dest->colors = NULL;
        dest->colors_size = 0;
    }
}

