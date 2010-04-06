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

#include "termit.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"
#include "keybindings.h"
#include "termit_core_api.h"
#include "termit_style.h"

void termit_create_popup_menu();
void termit_create_menubar();

static void termit_hide_scrollbars()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        if (!pTab->scrollbar_is_shown)
            gtk_widget_hide(pTab->scrollbar);
    }
}

void termit_tab_set_color_foreground(struct TermitTab* pTab, const GdkColor* p_color)
{
    pTab->style.foreground_color = *p_color;
    vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), &pTab->style.foreground_color);
}

void termit_tab_set_color_background(struct TermitTab* pTab, const GdkColor* p_color)
{
    pTab->style.background_color = *p_color;
    vte_terminal_set_color_background(VTE_TERMINAL(pTab->vte), &pTab->style.background_color);
}

static void termit_set_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        if (configs.style.colormap) {
            termit_tab_set_colormap(pTab, configs.style.colormap);
        } else {
            termit_tab_set_color_foreground(pTab, &configs.style.foreground_color);
            termit_tab_set_color_background(pTab, &configs.style.background_color);
        }
    }
}

static void termit_set_fonts()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint minWidth = 0, minHeight = 0;
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        vte_terminal_set_font(VTE_TERMINAL(pTab->vte), pTab->style.font);
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
    TRACE("width=%d height=%d", width, height);
    TRACE("minWidth=%d minHeight=%d", minWidth, minHeight);
    gtk_window_set_geometry_hints(GTK_WINDOW(termit.main_window), termit.main_window, &geom, GDK_HINT_MIN_SIZE);
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

void termit_after_show_all()
{
    termit_set_fonts();
    termit_hide_scrollbars();
    termit_set_colors();
    termit_toggle_menubar();
}

void termit_reconfigure()
{
    gtk_widget_destroy(termit.menu);
    gtk_container_remove(GTK_CONTAINER(termit.hbox), termit.menu_bar);

    termit_config_deinit();
    termit_configs_set_defaults();
    termit_keys_set_defaults();
    termit_lua_load_config();
    
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
    g_array_free(pTab->matches, TRUE);
    g_free(pTab->encoding);
    g_free(pTab->command);
    g_free(pTab->title);
    termit_style_free(&pTab->style);
    g_free(pTab);
    gtk_notebook_remove_page(GTK_NOTEBOOK(termit.notebook), page);

    termit_check_single_tab();
}

static void termit_tab_add_matches(struct TermitTab* pTab, GArray* matches)
{
    gint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        struct Match tabMatch = {0};
        tabMatch.lua_callback = match->lua_callback;
        tabMatch.pattern = match->pattern;
        tabMatch.tag = vte_terminal_match_add_gregex(VTE_TERMINAL(pTab->vte), match->regex, match->flags);
        vte_terminal_match_set_cursor_type(VTE_TERMINAL(pTab->vte), tabMatch.tag, GDK_HAND2);
        g_array_append_val(pTab->matches, tabMatch);
    }
}

void termit_tab_set_transparency(struct TermitTab* pTab, gdouble transparency)
{
    pTab->style.transparency = transparency;
    if (transparency) {
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), pTab->style.transparency);
    } else {
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), pTab->style.transparency);
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
    }
}

void termit_tab_set_audible_bell(struct TermitTab* pTab, gboolean audible_bell)
{
    pTab->audible_bell = audible_bell;
    vte_terminal_set_audible_bell(VTE_TERMINAL(pTab->vte), audible_bell);
}

void termit_tab_set_visible_bell(struct TermitTab* pTab, gboolean visible_bell)
{
    pTab->visible_bell = visible_bell;
    vte_terminal_set_visible_bell(VTE_TERMINAL(pTab->vte), visible_bell);
}

// Tango Color Theme
GdkColor colors[] = {
    {0, 0x2e2e, 0x3434, 0x3636},
    {0, 0xcccc, 0x0000, 0x0000},
    {0, 0x4e4e, 0x9a9a, 0x0606},
    {0, 0xc4c4, 0xa0a0, 0x0000},
    {0, 0x3434, 0x6565, 0xa4a4},
    {0, 0x7575, 0x5050, 0x7b7b},
    {0, 0x0606, 0x9820, 0x9a9a},
    {0, 0xd3d3, 0xd7d7, 0xcfcf},
    {0, 0x5555, 0x5757, 0x5353},
    {0, 0xefef, 0x2929, 0x2929},
    {0, 0x8a8a, 0xe2e2, 0x3434},
    {0, 0xfcfc, 0xe9e9, 0x4f4f},
    {0, 0x7272, 0x9f9f, 0xcfcf},
    {0, 0xadad, 0x7f7f, 0xa8a8},
    {0, 0x3434, 0xe2e2, 0xe2e2},
    {0, 0xeeee, 0xeeee, 0xecec}
};

void termit_append_tab_with_details(const struct TabInfo* ti)
{
    TRACE("%s", __FUNCTION__);
    struct TermitTab* pTab = g_malloc0(sizeof(struct TermitTab));
    termit_style_copy(&pTab->style, &configs.style);
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
        ERROR("%s", _("Cannot parse command. Creating tab with shell"));
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

    g_signal_connect(G_OBJECT(pTab->vte), "beep", G_CALLBACK(termit_on_beep), pTab);
    g_signal_connect(G_OBJECT(pTab->vte), "focus-in-event", G_CALLBACK(termit_on_focus), pTab);
    g_signal_connect(G_OBJECT(pTab->vte), "window-title-changed", G_CALLBACK(termit_on_tab_title_changed), NULL);

    g_signal_connect(G_OBJECT(pTab->vte), "child-exited", G_CALLBACK(termit_on_child_exited), NULL);
//    g_signal_connect(G_OBJECT(pTab->vte), "eof", G_CALLBACK(termit_eof), NULL);
    g_signal_connect_swapped(G_OBJECT(pTab->vte), "button-press-event", G_CALLBACK(termit_on_popup), NULL);
    
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), pTab->encoding);

    pTab->matches = g_array_new(FALSE, TRUE, sizeof(struct Match));
    termit_tab_add_matches(pTab, configs.matches);
    termit_tab_set_transparency(pTab, pTab->style.transparency);
    vte_terminal_set_font(VTE_TERMINAL(pTab->vte), pTab->style.font);

    gint index = gtk_notebook_append_page(GTK_NOTEBOOK(termit.notebook), pTab->hbox, pTab->tab_name);
    if (index == -1) {
        ERROR("%s", _("Cannot create a new tab"));
        return;
    }
    TRACE("index=%d, encoding=%s", index, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
    if (configs.fill_tabbar)
        gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE, TRUE, GTK_PACK_START);

    termit_tab_set_audible_bell(pTab, configs.audible_bell);
    termit_tab_set_visible_bell(pTab, configs.visible_bell);

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
    
    if (configs.style.colormap) {
        termit_tab_set_colormap(pTab, configs.style.colormap);
    } else {
        termit_tab_set_color_foreground(pTab, &pTab->style.foreground_color);
        termit_tab_set_color_background(pTab, &pTab->style.background_color);
    }

    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE);
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

void termit_tab_set_title(struct TermitTab* pTab, const gchar* title)
{
    gchar* tmp_title = g_strdup(title);
    if (configs.get_tab_title_callback) {
        gchar* lua_title = termit_lua_getTitleCallback(configs.get_tab_title_callback, title);
        if (!lua_title) {
            ERROR("termit_lua_getTitleCallback(%s) failed", title);
            g_free(tmp_title);
            return;
        }
        g_free(tmp_title);
        tmp_title = lua_title;
    }
    if (pTab->title)
        g_free(pTab->title);
    pTab->title = tmp_title;
    gtk_label_set_text(GTK_LABEL(pTab->tab_name), pTab->title);
    termit_set_window_title(title);
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

void termit_tab_set_font(struct TermitTab* pTab, const gchar* font_name)
{
    if (pTab->style.font_name) {
        g_free(pTab->style.font_name);
    }
    pTab->style.font_name = g_strdup(font_name);

    if (pTab->style.font) {
        pango_font_description_free(pTab->style.font);
    }
    pTab->style.font = pango_font_description_from_string(font_name);
    vte_terminal_set_font(VTE_TERMINAL(pTab->vte), pTab->style.font);
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

void termit_tab_set_color_foreground_by_index(gint tab_index, const GdkColor* p_color)
{
    termit_set_color__(tab_index, p_color, termit_tab_set_color_foreground);
}

void termit_tab_set_color_background_by_index(gint tab_index, const GdkColor* p_color)
{
    termit_set_color__(tab_index, p_color, termit_tab_set_color_background);
}

void termit_tab_set_colormap(struct TermitTab* pTab, const GdkColormap* p_colormap)
{
    vte_terminal_set_colors(VTE_TERMINAL(pTab->vte), NULL, NULL, p_colormap->colors, p_colormap->size);
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
    
    gtk_main_quit();
}

int termit_get_current_tab_index()
{
    return gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
}

void termit_set_kb_policy(enum TermitKbPolicy kbp)
{
    configs.kb_policy = kbp;
}

void termit_set_window_title(const gchar* title)
{
    if (!title)
        return;
    if (!configs.get_window_title_callback) {
        gchar* window_title = g_strdup_printf("%s: %s", configs.default_window_title, title);
        gtk_window_set_title(GTK_WINDOW(termit.main_window), window_title);
        g_free(window_title);
    } else {
        gchar* window_title = termit_lua_getTitleCallback(configs.get_window_title_callback, title);
        if (!window_title) {
            ERROR("termit_lua_getTitleCallback(%s) failed", title);
            return;
        }
        gtk_window_set_title(GTK_WINDOW(termit.main_window), window_title);
        g_free(window_title);
    }
}

