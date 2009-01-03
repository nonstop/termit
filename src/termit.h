#ifndef TERMIT_H
#define TERMIT_H


#include <config.h>

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib/gprintf.h>

#include <libintl.h>
#define _(String) gettext(String)

struct TermitData
{
    GtkWidget *main_window;
    GtkWidget *notebook;
    GtkWidget *statusbar;
    GtkWidget *cb_bookmarks;
    GtkWidget *menu;
    GtkWidget *mi_show_scrollbar;
    GtkWidget *hbox;
    GtkWidget *menu_bar;
    gint tab_max_number;
    PangoFontDescription *font;
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
    gchar *encoding;
    gchar *command;
    gchar *title;
    GdkColor foreground_color;
    GdkColor background_color;
    pid_t pid;
    gulong sig_wtc;
};

struct TabInfo
{
    gchar* name;
    gchar* command;
    gchar* working_dir;
    gchar* encoding;
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

//#define ERROR(x) g_fprintf(stderr, "%s:%d error: %s\n", __FILE__, __LINE__, x)
#define ERROR(format, ...) g_fprintf(stderr, format, ## __VA_ARGS__); g_fprintf(stderr, "\n")

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

