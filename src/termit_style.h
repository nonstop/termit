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

#ifndef TERMIT_STYLE_H
#define TERMIT_STYLE_H

#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

struct TermitStyle
{
    gchar* font_name;
    PangoFontDescription* font;
    GdkRGBA* foreground_color;
    GdkRGBA* background_color;
    GdkRGBA* colors;
    glong colors_size;
};

void termit_style_init(struct TermitStyle* style);
void termit_style_copy(struct TermitStyle* dest, const struct TermitStyle* src);
void termit_style_free(struct TermitStyle* style);

#endif /* TERMIT_STYLE_H */
