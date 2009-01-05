
#include "termit.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"
#include "keybindings.h"
#include "termit_core_api.h"

void termit_create_popup_menu();
void termit_create_menubar();

void termit_after_show_all()
{
    termit_hide_scrollbars();
    termit_set_colors();
    termit_toggle_menubar();
}

void termit_reconfigure()
{
    gtk_widget_destroy(termit.menu);
    gtk_container_remove(GTK_CONTAINER(termit.hbox), termit.menu_bar);

    termit_deinit_config();
    termit_set_default_options();
    termit_set_default_keybindings();
    termit_load_lua_config();
    
    termit_create_popup_menu();
    termit_create_menubar();
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.menu_bar, FALSE, 0, 0);
    gtk_box_reorder_child(GTK_BOX(termit.hbox), termit.menu_bar, 0);
    gtk_widget_show_all(termit.main_window);
}

void termit_set_statusbar_encoding(gint page)
{
    if (page < 0)
        page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);

    gtk_statusbar_push(GTK_STATUSBAR(termit.statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
}

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

static void termit_del_tab()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
            
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    TRACE("%s pid=%d", __FUNCTION__, pTab->pid);
    g_free(pTab->encoding);
    g_free(pTab->command);
    g_free(pTab->title);
    g_free(pTab);
    gtk_notebook_remove_page(GTK_NOTEBOOK(termit.notebook), page);
    
    termit_check_single_tab();
}

void termit_hide_scrollbars()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        if (!pTab->scrollbar_is_shown)
            gtk_widget_hide(pTab->scrollbar);
    }
}

static void termit_set_tab_foreground_color__(struct TermitTab* pTab, const GdkColor* p_color)
{
    pTab->foreground_color = *p_color;
    {
        gchar* color_str = gdk_color_to_string(p_color);
        TRACE("color: [%s]", color_str);
        g_free(color_str);
    }
    vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), &pTab->foreground_color);
}

static void termit_set_tab_background_color__(struct TermitTab* pTab, const GdkColor* p_color)
{
    pTab->background_color = *p_color;
    {
        gchar* color_str = gdk_color_to_string(p_color);
        TRACE("color: [%s]", color_str);
        g_free(color_str);
    }
    vte_terminal_set_color_background(VTE_TERMINAL(pTab->vte), &pTab->background_color);
}

void termit_set_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        if (configs.default_foreground_color)
            termit_set_tab_foreground_color__(pTab, configs.default_foreground_color);
        if (configs.default_background_color)
            termit_set_tab_background_color__(pTab, configs.default_background_color);
    }
}

void termit_append_tab_with_details(const struct TabInfo* ti)
{
    TRACE("%s", __FUNCTION__);
    struct TermitTab* pTab = g_malloc0(sizeof(struct TermitTab));

    if (ti->name) {
        pTab->tab_name = gtk_label_new(ti->name);
        pTab->custom_tab_name = TRUE;
    } else {
        gchar* label_text = g_strdup_printf("%s %d", configs.default_tab_name, termit.tab_max_number++);
        pTab->tab_name = gtk_label_new(label_text);
        g_free(label_text);
        pTab->custom_tab_name = FALSE;
    }
    pTab->encoding = (ti->encoding) ? g_strdup(ti->encoding) : g_strdup(configs.default_encoding);

    pTab->hbox = gtk_hbox_new(FALSE, 0);
    pTab->vte = vte_terminal_new();

    vte_terminal_set_scrollback_lines(VTE_TERMINAL(pTab->vte), configs.scrollback_lines);
    if (configs.default_word_chars)
        vte_terminal_set_word_chars(VTE_TERMINAL(pTab->vte), configs.default_word_chars);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(pTab->vte), TRUE);

    /* parse command */
    gchar **cmd_argv;
    GError *cmd_err = NULL;
    gchar *cmd_path = NULL;
    gchar *cmd_file = NULL;

    pTab->command = (ti->command) ? g_strdup(ti->command) : g_strdup(configs.default_command);
    TRACE("command=%s", pTab->command);
    if (!g_shell_parse_argv(pTab->command, NULL, &cmd_argv, &cmd_err)) {
        ERROR(_("Cannot parse command. Creating tab with shell"));
        g_error_free(cmd_err);
    } else {
        cmd_path = g_find_program_in_path(cmd_argv[0]);
        cmd_file = g_path_get_basename(cmd_argv[0]);
    }

    if (cmd_path && cmd_file) {
        g_free(cmd_argv[0]);
        cmd_argv[0] = g_strdup(cmd_file);

        pTab->pid = vte_terminal_fork_command(VTE_TERMINAL(pTab->vte),
                cmd_path, cmd_argv, NULL, ti->working_dir, TRUE, TRUE, TRUE);
    } else {
        /* default tab */
        pTab->pid = vte_terminal_fork_command(VTE_TERMINAL(pTab->vte),
                configs.default_command, NULL, NULL, ti->working_dir, TRUE, TRUE, TRUE);
    }

    g_strfreev(cmd_argv);
    g_free(cmd_path);
    g_free(cmd_file);

    g_signal_connect(G_OBJECT(pTab->vte), "window-title-changed", G_CALLBACK(termit_on_window_title_changed), NULL);

    g_signal_connect(G_OBJECT(pTab->vte), "child-exited", G_CALLBACK(termit_on_child_exited), NULL);
//    g_signal_connect(G_OBJECT(pTab->vte), "eof", G_CALLBACK(termit_eof), NULL);
    g_signal_connect_swapped(G_OBJECT(pTab->vte), "button-press-event", G_CALLBACK(termit_on_popup), NULL);
    
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), pTab->encoding);

    if (configs.transparent_background) {
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), configs.transparent_saturation);
    }
    vte_terminal_set_font(VTE_TERMINAL(pTab->vte), termit.font);    

    gint index = gtk_notebook_append_page(GTK_NOTEBOOK(termit.notebook), pTab->hbox, pTab->tab_name);
    if (index == -1) {
        ERROR(_("Cannot create a new tab"));
        return;
    }
    TRACE("index=%d, encoding=%s", index, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
    if (configs.fill_tabbar)
        gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE, TRUE, GTK_PACK_START);
    
    pTab->scrollbar = gtk_vscrollbar_new(vte_terminal_get_adjustment(VTE_TERMINAL(pTab->vte)));

    gtk_box_pack_start(GTK_BOX(pTab->hbox), pTab->vte, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pTab->hbox), pTab->scrollbar, FALSE, FALSE, 0);
    GtkWidget* tabWidget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(termit.notebook), index);
    if (!tabWidget) {
        ERROR("tabWidget is NULL");
        return;
    }
    g_object_set_data(G_OBJECT(tabWidget), "termit.tab", pTab);

    pTab->scrollbar_is_shown = configs.show_scrollbar;
    gtk_widget_show_all(termit.notebook);
    
    if (configs.default_foreground_color)
        termit_set_tab_foreground_color__(pTab, configs.default_foreground_color);
    if (configs.default_background_color)
        termit_set_tab_background_color__(pTab, configs.default_background_color);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index);
#if GTK_CHECK_VERSION(2,10,0)
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE);
#endif
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);

    termit_set_statusbar_encoding(-1);
    
    termit_check_single_tab();
    termit_hide_scrollbars();
}

void termit_append_tab_with_command(const gchar* command)
{
    struct TabInfo ti = {0};
    ti.command = g_strdup(command);
    termit_append_tab_with_details(&ti);
    g_free(ti.command);
}

void termit_append_tab()
{
    termit_append_tab_with_command(configs.default_command);
}

void termit_toggle_menubar()
{
    static int menubar_visible = TRUE;
    static int first_run = TRUE;
    if (first_run) {
        menubar_visible = !configs.hide_menubar;
        first_run = FALSE;
    }
    if (menubar_visible)
        gtk_widget_show(GTK_WIDGET(termit.hbox));
    else
        gtk_widget_hide(GTK_WIDGET(termit.hbox));
    menubar_visible = !menubar_visible;
}

void termit_set_encoding(const gchar* encoding)
{
    TRACE_MSG(__FUNCTION__);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), encoding);
    g_free(pTab->encoding);
    pTab->encoding = g_strdup(encoding);
    termit_set_statusbar_encoding(-1);
}

void termit_set_tab_name(guint tab_index, const gchar* name)
{
    TERMIT_GET_TAB_BY_INDEX(pTab, tab_index);
    gtk_label_set_text(GTK_LABEL(pTab->tab_name), name);
    pTab->custom_tab_name = TRUE;
    termit_on_window_title_changed(VTE_TERMINAL(pTab->vte), NULL);
}

void termit_set_default_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        vte_terminal_set_default_colors(VTE_TERMINAL(pTab->vte));
    }
}

static void termit_set_color__(gint tab_index, const GdkColor* p_color, void (*callback)(struct TermitTab*, const GdkColor*))
{
    TRACE("%s: tab_index=%d color=%p", __FUNCTION__, tab_index, p_color);
    if (!p_color) {
        TRACE_MSG("p_color is NULL");
        return;
    }
    if (tab_index < 0) {
        tab_index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    }
    TERMIT_GET_TAB_BY_INDEX(pTab, tab_index);
    callback(pTab, p_color);
}

void termit_set_tab_foreground_color(gint tab_index, const GdkColor* p_color)
{
    termit_set_color__(tab_index, p_color, termit_set_tab_foreground_color__);
}

void termit_set_tab_background_color(gint tab_index, const GdkColor* p_color)
{
    termit_set_color__(tab_index, p_color, termit_set_tab_background_color__);
}

void termit_set_font(const gchar* font_name)
{
    TRACE("%s: font_name=%s", __FUNCTION__, font_name);

    pango_font_description_free(termit.font);
    termit.font = pango_font_description_from_string(font_name);

    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint minWidth = 0, minHeight = 0;
    /* Set the font for all tabs */
    gint i=0;
    for (; i<page_num; ++i)
    {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
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

void termit_paste()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    vte_terminal_paste_clipboard(VTE_TERMINAL(pTab->vte));
}

void termit_copy()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    vte_terminal_copy_clipboard(VTE_TERMINAL(pTab->vte));
}

void termit_close_tab()
{
    termit_del_tab();
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 0)
        termit_quit();
}

void termit_activate_tab(gint tab_index)
{
    if (tab_index < 0)
    {
        TRACE("tab_index(%d) < 0: skipping", tab_index);
        return;
    }
    if (tab_index >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)))
    {
        TRACE("tab_index(%d) > n_pages: skipping", tab_index);
        return;
    }
    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), tab_index);
}

void termit_prev_tab()
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    index = (index) ? index - 1 : gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) - 1;
    termit_activate_tab(index);
}

void termit_next_tab()
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    index = (index == gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) - 1)
        ? 0 : index + 1;
    termit_activate_tab(index);
}

void termit_quit()
{
    while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) > 0)
        termit_del_tab();
    
    pango_font_description_free(termit.font);

    gtk_main_quit();
}

void termit_set_kb_policy(enum TermitKbPolicy kbp)
{
    configs.kb_policy = kbp;
}

void termit_set_window_title(const gchar* title)
{
    if (!title)
        return;
    gtk_window_set_title(GTK_WINDOW(termit.main_window), title);
}

