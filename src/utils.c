
#include "utils.h"
#include "callbacks.h"
#include "configs.h"

extern struct TermitData termit;
extern struct Configs configs;

static void termit_check_single_tab()
{
    if (configs.hide_single_tab)
    {
        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 1)
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), FALSE);
        else
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);
    }
}

void termit_append_tab_with_details(const gchar* tab_name, const gchar* shell_cmd, 
    const gchar* working_dir, const gchar* encoding)
{
    TRACE("%s", __FUNCTION__);
    struct TermitTab* pTab = g_malloc(sizeof(struct TermitTab));

    pTab->tab_name = gtk_label_new(tab_name);
    pTab->encoding = g_strdup(encoding);

    pTab->hbox = gtk_hbox_new(FALSE, 0);
    pTab->vte = vte_terminal_new();

    vte_terminal_set_scrollback_lines(VTE_TERMINAL(pTab->vte), configs.scrollback_lines);
    if (configs.default_word_chars)
    {
        TRACE("%s", configs.default_word_chars);
        vte_terminal_set_word_chars(VTE_TERMINAL(pTab->vte), configs.default_word_chars);
    }
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(pTab->vte), TRUE);

    pTab->scrollbar = gtk_vscrollbar_new(vte_terminal_get_adjustment(VTE_TERMINAL(pTab->vte)));

    gtk_box_pack_start(GTK_BOX(pTab->hbox), pTab->vte, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pTab->hbox), pTab->scrollbar, FALSE, FALSE, 0);

    /* parse command */
    gchar **cmd_argv;
    GError *cmd_err = NULL;
    gchar *cmd_path = NULL;
    gchar *cmd_file = NULL;

    TRACE("shell_cmd=%s", shell_cmd);
    if (!g_shell_parse_argv(shell_cmd, NULL, &cmd_argv, &cmd_err))
    {
        ERROR(_("Cannot parse command. Creating tab with shell"));
        g_error_free(cmd_err);
    }
    else
    {
        cmd_path = g_find_program_in_path(cmd_argv[0]);
        cmd_file = g_path_get_basename(cmd_argv[0]);
    }

    if (cmd_path && cmd_file)
    {
        g_free(cmd_argv[0]);
        cmd_argv[0] = g_strdup(cmd_file);

        pTab->pid = vte_terminal_fork_command(VTE_TERMINAL(pTab->vte),
                cmd_path, cmd_argv, NULL, working_dir, TRUE, TRUE, TRUE);
    }
    else
    {
        /* default tab */
        pTab->pid = vte_terminal_fork_command(VTE_TERMINAL(pTab->vte),
                g_getenv("SHELL"), NULL, NULL, working_dir, TRUE, TRUE, TRUE);
    }

    g_strfreev(cmd_argv);
    g_free(cmd_path);
    g_free(cmd_file);

    int index = gtk_notebook_append_page(GTK_NOTEBOOK(termit.notebook), pTab->hbox, pTab->tab_name);
    if (index == -1)
    {
        ERROR(_("Cannot create a new tab"));
        return;
    }

    g_signal_connect(G_OBJECT(pTab->vte), "child-exited", G_CALLBACK(termit_child_exited), NULL);
    g_signal_connect(G_OBJECT(pTab->vte), "eof", G_CALLBACK(termit_eof), NULL);
    g_signal_connect_swapped(G_OBJECT(pTab->vte), "button-press-event", G_CALLBACK(termit_popup), termit.menu);
    
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), pTab->encoding);

    TRACE("index=%d, encoding=%s", index, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
    
    if (configs.transparent_background)
    {
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), configs.transparent_saturation);
    }

    GtkWidget* tabWidget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(termit.notebook), index);
    if (!tabWidget)
    {
        ERROR("tabWidget is NULL");
        return;
    }
    g_object_set_data(G_OBJECT(tabWidget), "termit.tab", pTab);

    if (configs.show_scrollbar)
    {
        pTab->scrollbar_is_shown = TRUE;
        gtk_widget_show_all(termit.notebook);
    }
    else
    {
        TRACE(hide);
        pTab->scrollbar_is_shown = FALSE;
        gtk_widget_show(termit.notebook);
        gtk_widget_show(pTab->hbox);
        gtk_widget_show(pTab->tab_name);
        gtk_widget_show(pTab->vte);
        gtk_widget_hide(pTab->scrollbar);
    }

    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index);
#if GTK_CHECK_VERSION(2,10,0)
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE);
#endif
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);

    vte_terminal_set_font(VTE_TERMINAL(pTab->vte), termit.font);    
    termit_set_statusbar_encoding(-1);

    termit_check_single_tab();
}

void termit_append_tab_with_command(const gchar* command)
{
    gchar *label_text = g_strdup_printf("%s %d", configs.default_tab_name, termit.tab_max_number++);

    termit_append_tab_with_details(label_text, command, g_getenv("PWD"), configs.default_encoding);
    g_free(label_text);
}

void termit_append_tab()
{
    termit_append_tab_with_command(g_getenv("SHELL"));
}

void termit_set_font()
{
    TRACE("%s", __FUNCTION__);
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint minWidth = 0, minHeight = 0;
    /* Set the font for all tabs */
    int i=0;
    for (i=0; i<page_num; i++)
    {
        TERMIT_GET_TAB_BY_INDEX(pTab, i)
        vte_terminal_set_font(VTE_TERMINAL(pTab->vte), termit.font);
        gint xpad = 0, ypad = 0;
        vte_terminal_get_padding(VTE_TERMINAL(pTab->vte), &xpad, &ypad);
        gint w = vte_terminal_get_char_width(VTE_TERMINAL(pTab->vte)) * configs.cols + xpad;
        if (w > minWidth)
            minWidth = w;
        gint h = vte_terminal_get_char_height(VTE_TERMINAL(pTab->vte)) * configs.rows + ypad;
        if (h > minHeight)
            minHeight = h;
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
    TERMIT_GET_TAB_BY_INDEX(pTab, page)

    gtk_statusbar_push(GTK_STATUSBAR(termit.statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
}

gchar* termit_get_pid_dir(pid_t pid)
{
    gchar* file = g_strdup_printf("/proc/%d/cwd", pid);
    gchar* link = g_file_read_link(file, NULL);
    g_free(file);
    return link;
}

void termit_del_tab()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
            
    TERMIT_GET_TAB_BY_INDEX(pTab, page)
    TRACE("%s pid=%d", __FUNCTION__, pTab->pid);
    g_free(pTab->encoding);
    g_free(pTab);

    gtk_notebook_remove_page(GTK_NOTEBOOK(termit.notebook), page);
    
    termit_check_single_tab();
}

struct TermitTab* termit_get_tab_by_index(gint index)
{
    GtkWidget* tabWidget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(termit.notebook), index);
    if (!tabWidget)
    {
        ERROR("tabWidget is NULL");
        return NULL;
    }
    struct TermitTab* pTab = (struct TermitTab*)g_object_get_data(G_OBJECT(tabWidget), "termit.tab");
    return pTab;
}

