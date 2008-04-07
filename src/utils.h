#ifndef UTILS_H
#define UTILS_H

#include <config.h>

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib/gprintf.h>

#include <libintl.h>
#define _(String) gettext(String)

//#ifdef DEBUG
//#define TRACE g_fprintf(stderr, "%s:%d\n", __FILE__, __LINE__)
//#define TRACE_MSG(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
//#define TRACE_STR(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
//#define TRACE_NUM(x) g_fprintf(stderr, "%s:%d %s = %d\n", __FILE__, __LINE__, #x, x)
//#define TRACE_FLT(x) g_fprintf(stderr, "%s:%d %s = %f\n", __FILE__, __LINE__, #x, x)
//#else
//#define TRACE 
//#define TRACE_MSG(x)
//#define TRACE_STR(x) 
//#define TRACE_NUM(x) 
//#endif // DEBUG
#define ERROR(x) g_fprintf(stderr, "%s:%d error: %s\n", __FILE__, __LINE__, x)

#ifdef DEBUG
#define STDFMT " %s:%d "
#define STD __FILE__, __LINE__
#define TRACE(format, ...) g_fprintf(stderr, STDFMT # format, STD, ## __VA_ARGS__); g_fprintf(stderr, "\n")
#define TRACE_MSG(x) g_fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, x)
#else
#define TRACE(format, ...)
#define TRACE_MSG(x)
#endif

struct TermitData
{
    GtkWidget *main_window;
    GtkWidget *notebook;
    GtkWidget *statusbar;
    GtkWidget *cb_bookmarks;
    GtkWidget *menu;
    GtkWidget *menu_bar;
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


struct TermitTab* termit_get_tab_by_index(gint index);
#define TERMIT_GET_TAB_BY_INDEX(pTab, ind) \
    struct TermitTab* pTab = termit_get_tab_by_index(ind); \
    if (!pTab) \
    {   g_fprintf(stderr, "%s:%d error: %s is null\n", __FILE__, __LINE__, #pTab); return; }
#define TERMIT_GET_TAB_BY_INDEX2(pTab, ind, retCode) \
    struct TermitTab* pTab = termit_get_tab_by_index(ind); \
    if (!pTab) \
    {   g_fprintf(stderr, "%s:%d error: %s is null\n", __FILE__, __LINE__, #pTab); return retCode; }

#endif /* UTILS_H */

