#ifndef UTILS_H
#define UTILS_H

#include <config.h>

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib/gprintf.h>

#include <libintl.h>
#define _(String) gettext(String)

#ifdef DEBUG
#define TRACE g_fprintf(stderr, "%s:%d\n", __FILE__, __LINE__)
#define TRACE_MSG(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
#define TRACE_STR(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
#define TRACE_NUM(x) g_fprintf(stderr, "%s:%d %s = %d\n", __FILE__, __LINE__, #x, x)
#define ERROR(x) g_fprintf(stderr, "%s:%d error: %s\n", __FILE__, __LINE__, x)
#else
#define TRACE 
#define TRACE_MSG(x)
#define TRACE_STR(x) 
#define TRACE_NUM(x) 
#define ERROR(x) 
#endif // DEBUG

struct TermitData
{
    GtkWidget *main_window;
    GtkWidget *notebook;
    GtkWidget *statusbar;
    GtkWidget *cb_bookmarks;
    GtkWidget *menu;
    GtkWidget *menu_bar;
    GArray *tabs;
    gint tab_max_number;
    PangoFontDescription *font;
};

struct TermitTab
{
    GtkWidget *tab_name;
    GtkWidget *hbox;
    GtkWidget *vte;
    GtkWidget *scrollbar;
    gchar *encoding;
    pid_t pid;
};

#endif /* UTILS_H */

