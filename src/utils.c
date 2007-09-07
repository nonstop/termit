
#include "utils.h"
#include "callbacks.h"
#include "configs.h"

extern struct TermitData termit;
extern struct Configs configs;

void termit_append_tab()
{
    TRACE;
    struct TermitTab tab;
    gchar *label_text = g_strdup_printf("%s %d", configs.default_tab_name, termit.tab_max_number++);

    tab.tab_name = gtk_label_new(label_text);
    tab.encoding = g_strdup(configs.default_encoding);
    g_free(label_text);
    
    tab.hbox = gtk_hbox_new(FALSE, 0);
    tab.vte = vte_terminal_new();

    vte_terminal_set_scrollback_lines(VTE_TERMINAL(tab.vte), configs.scrollback_lines);
    TRACE_STR(configs.default_word_chars);
    vte_terminal_set_word_chars(VTE_TERMINAL(tab.vte), configs.default_word_chars);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(tab.vte), TRUE);
    
    tab.scrollbar = gtk_vscrollbar_new(vte_terminal_get_adjustment(VTE_TERMINAL(tab.vte)));

    gtk_box_pack_start(GTK_BOX(tab.hbox), tab.vte, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab.hbox), tab.scrollbar, FALSE, FALSE, 0);


    tab.pid = vte_terminal_fork_command(VTE_TERMINAL(tab.vte), 
            g_getenv("SHELL"), NULL, NULL, 
            g_getenv("PWD"), TRUE, TRUE,TRUE);
    int index = gtk_notebook_append_page(GTK_NOTEBOOK(termit.notebook), tab.hbox, tab.tab_name);
    if (index ==-1)
    {
        ERROR("Cannot create a new tab");
        return;
    }
    g_signal_connect(G_OBJECT(tab.vte), "child-exited", G_CALLBACK(termit_child_exited), NULL);
    g_signal_connect(G_OBJECT(tab.vte), "eof", G_CALLBACK(termit_eof), NULL);
	g_signal_connect_swapped(G_OBJECT(tab.vte), "button-press-event", G_CALLBACK(termit_popup), termit.menu);
    
    TRACE_NUM(index);
    TRACE_STR(vte_terminal_get_encoding(VTE_TERMINAL(tab.vte)));
    g_array_append_val(termit.tabs, tab);
    
    gtk_widget_show_all(termit.notebook);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index);
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), tab.vte);
    
    vte_terminal_set_font(VTE_TERMINAL(tab.vte), termit.font);
}

void termit_set_font()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    struct TermitTab tab;
    gint minWidth, minHeight;
    /* Set the font for all tabs */
    int i=0;
    for (i=0; i<page_num; i++)
    {
        tab = g_array_index(termit.tabs, struct TermitTab, i);	
        vte_terminal_set_font(VTE_TERMINAL(tab.vte), termit.font);
        minWidth = vte_terminal_get_char_width(VTE_TERMINAL(tab.vte))*80;
        minHeight = vte_terminal_get_char_height(VTE_TERMINAL(tab.vte))*24;
    }
    gint oldWidth, oldHeight;
    gtk_window_get_size(GTK_WINDOW(termit.main_window), &oldWidth, &oldHeight);
    
    gint width = (minWidth > oldWidth) ? minWidth : oldWidth;
    gint height = (minHeight > oldHeight) ? minHeight : oldHeight;
    gtk_window_resize(GTK_WINDOW(termit.main_window), width, height);

    GdkGeometry geom;
    geom.min_width = minWidth;
    geom.min_height = minHeight;
    gtk_window_set_geometry_hints(GTK_WINDOW(termit.main_window), termit.main_window, &geom, GDK_HINT_MIN_SIZE);
}

void termit_set_statusbar_encoding(gint page)
{
    if (page < 0)
        page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    struct TermitTab tab = g_array_index(termit.tabs, struct TermitTab, page);

    gchar *statusbar_text = g_strdup_printf("%s: %s", _("Encoding"), vte_terminal_get_encoding(VTE_TERMINAL(tab.vte)));
    gtk_statusbar_push(GTK_STATUSBAR(termit.statusbar), 0, statusbar_text);
    g_free(statusbar_text);
}

