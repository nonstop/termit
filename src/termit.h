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

#ifndef TERMIT_H
#define TERMIT_H


#include <config.h>

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib/gprintf.h>

#include <libintl.h>
#define _(String) gettext(String)

#include "termit_style.h"

struct TermitData
{
    GtkWidget *main_window;
    GtkWidget *notebook;
    GtkWidget *statusbar;
    GtkWidget *b_toggle_search;
    GtkWidget *b_find_next;
    GtkWidget *b_find_prev;
    GtkWidget *search_entry;
    GtkWidget *cb_bookmarks;
    GtkWidget *menu;
    GtkWidget *mi_show_scrollbar;
    GtkWidget *hbox;
    GtkWidget *menu_bar;
    gint tab_max_number;
};
extern struct TermitData termit;

struct TermitTab
{
    GtkWidget *tab_name;
    GtkWidget *hbox;
    GtkWidget *vte;
    GtkWidget *scrollbar;
    gboolean scrollbar_is_shown;
    gboolean custom_tab_name;
    gboolean audible_bell;
    VteEraseBinding bksp_binding;
    VteEraseBinding delete_binding;
    VteCursorBlinkMode cursor_blink_mode;
    VteCursorShape cursor_shape;
    gchar *encoding;
    gchar **argv;
    gchar *title;
    GArray* matches;
    gchar* search_regex;
    struct TermitStyle style;
    GPid pid;
    gulong onChildExitedHandlerId;
};

struct TabInfo
{
    gchar* name;
    gchar** argv;
    gchar* working_dir;
    gchar* encoding;
    VteEraseBinding bksp_binding;
    VteEraseBinding delete_binding;
    VteCursorBlinkMode cursor_blink_mode;
    VteCursorShape cursor_shape;
};

struct TermitTab* termit_get_tab_by_index(guint);
#define TERMIT_GET_TAB_BY_INDEX(pTab, ind, action) \
    struct TermitTab* pTab = termit_get_tab_by_index(ind); \
    if (!pTab) \
    {   g_fprintf(stderr, "%s:%d error: %s is null\n", __FILE__, __LINE__, #pTab); action; }
struct TermitTab* termit_get_tab_by_vte(VteTerminal*, gint*);

#ifdef DEBUG
#define ERROR(format, ...) g_fprintf(stderr, "ERROR: %s:%d " format "\n", __FILE__, __LINE__, ## __VA_ARGS__)
#define STDFMT "%s:%d "
#define STD __FILE__, __LINE__
#define TRACE(format, ...) g_fprintf(stderr, STDFMT format, STD, ## __VA_ARGS__); g_fprintf(stderr, "\n")
#define TRACE_MSG(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
#define TRACE_FUNC g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__)
#else
#define ERROR(format, ...) g_fprintf(stderr, "%s:%d " format "\n", __FILE__, __LINE__, ## __VA_ARGS__)
#define TRACE(format, ...)
#define TRACE_MSG(x)
#define TRACE_FUNC
#endif

#endif /* TERMIT_H */
