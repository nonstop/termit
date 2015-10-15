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
#ifdef TERMIT_ENABLE_SEARCH
    GtkWidget *b_toggle_search;
    GtkWidget *b_find_next;
    GtkWidget *b_find_prev;
    GtkWidget *search_entry;
#endif // TERMIT_ENABLE_SEARCH
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
    gboolean visible_bell;
    VteTerminalEraseBinding bksp_binding;
    VteTerminalEraseBinding delete_binding;
    gchar *encoding;
    gchar **argv;
    gchar *title;
    GArray* matches;
    struct TermitStyle style;
    pid_t pid;
};

struct TabInfo
{
    gchar* name;
    gchar** argv;
    gchar* working_dir;
    gchar* encoding;
    VteTerminalEraseBinding bksp_binding;
    VteTerminalEraseBinding delete_binding;
};

struct TermitTab* termit_get_tab_by_index(guint index);
#define TERMIT_GET_TAB_BY_INDEX(pTab, ind) \
    struct TermitTab* pTab = termit_get_tab_by_index(ind); \
    if (!pTab) \
    {   g_fprintf(stderr, "%s:%d error: %s is null\n", __FILE__, __LINE__, #pTab); return; }
#define TERMIT_GET_TAB_BY_INDEX2(pTab, ind, retCode) \
    struct TermitTab* pTab = termit_get_tab_by_index(ind); \
    if (!pTab) \
    {   g_fprintf(stderr, "%s:%d error: %s is null\n", __FILE__, __LINE__, #pTab); return retCode; }

//#define ERROR(x) g_fprintf(stderr, "%s:%d error: %s\n", __FILE__, __LINE__, x)
#define ERROR(format, ...) g_fprintf(stderr, "%s:%d " format "\n", __FILE__, __LINE__, ## __VA_ARGS__)

#ifdef DEBUG
#define STDFMT "%s:%d "
#define STD __FILE__, __LINE__
#define TRACE(format, ...) g_fprintf(stderr, STDFMT # format, STD, ## __VA_ARGS__); g_fprintf(stderr, "\n")
#define TRACE_MSG(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
#define TRACE_FUNC g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__)
#else
#define TRACE(format, ...)
#define TRACE_MSG(x)
#define TRACE_FUNC
#endif

#endif /* TERMIT_H */

