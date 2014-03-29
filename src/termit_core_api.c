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

#include <string.h>
#include <stdlib.h>
#include <math.h>
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
    if (p_color) {
        pTab->style.foreground_color = gdk_color_copy(p_color);
        vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        if (pTab->style.foreground_color) {
            vte_terminal_set_color_bold(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        }
    }
}

void termit_tab_set_color_background(struct TermitTab* pTab, const GdkColor* p_color)
{
    if (p_color) {
        pTab->style.background_color = gdk_color_copy(p_color);
        vte_terminal_set_color_background(VTE_TERMINAL(pTab->vte), pTab->style.background_color);
    }
}

void termit_tab_apply_colors(struct TermitTab* pTab)
{
    if (pTab->style.colors) {
        vte_terminal_set_colors(VTE_TERMINAL(pTab->vte), NULL, NULL, pTab->style.colors, pTab->style.colors_size);
        if (pTab->style.foreground_color) {
            vte_terminal_set_color_bold(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        }
    }
    if (pTab->style.foreground_color) {
        vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
    }
    if (pTab->style.background_color) {
        vte_terminal_set_color_background(VTE_TERMINAL(pTab->vte), pTab->style.background_color);
    }
}

void termit_tab_feed(struct TermitTab* pTab, const gchar* data)
{
    vte_terminal_feed(VTE_TERMINAL(pTab->vte), data, strlen(data));
}

void termit_tab_feed_child(struct TermitTab* pTab, const gchar* data)
{
    vte_terminal_feed_child(VTE_TERMINAL(pTab->vte), data, strlen(data));
}

static void termit_set_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        termit_tab_apply_colors(pTab);
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
        GtkBorder* border;
        gtk_widget_style_get(GTK_WIDGET(pTab->vte), "inner-border", &border, NULL);
        gint w = vte_terminal_get_char_width(VTE_TERMINAL(pTab->vte)) * configs.cols + border->left + border->right;
        if (w > minWidth)
            minWidth = w;
        gint h = vte_terminal_get_char_height(VTE_TERMINAL(pTab->vte)) * configs.rows + border->top + border->bottom;
        if (h > minHeight)
            minHeight = h;
    }
    gint oldWidth, oldHeight;
    gtk_window_get_size(GTK_WINDOW(termit.main_window), &oldWidth, &oldHeight);
    const gint width = (minWidth > oldWidth) ? minWidth : oldWidth;
    const gint height = (minHeight > oldHeight) ? minHeight : oldHeight;
    gtk_window_resize(GTK_WINDOW(termit.main_window), width, height);

    GdkGeometry geom;
    geom.min_width = minWidth;
    geom.min_height = minHeight;
    gtk_window_set_geometry_hints(GTK_WINDOW(termit.main_window), termit.main_window, &geom, GDK_HINT_MIN_SIZE);
}

gchar* termit_get_pid_dir(pid_t pid)
{
    gchar* file = g_strdup_printf("/proc/%d/cwd", pid);
    gchar* link = g_file_read_link(file, NULL);
    g_free(file);
    return link;
}


void termit_toggle_menubar()
{
    if (configs.hide_menubar) {
        gtk_widget_hide(GTK_WIDGET(termit.hbox));
    } else {
        gtk_widget_show(GTK_WIDGET(termit.hbox));
    }
    configs.hide_menubar = !configs.hide_menubar;
}

static void termit_check_tabbar()
{
    if (!configs.hide_tabbar) {
        if (configs.hide_single_tab && gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 1) {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), FALSE);
        } else {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);
        }
    } else {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), FALSE);
    }
}

void termit_toggle_tabbar()
{
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), configs.hide_tabbar);
    configs.hide_tabbar = !configs.hide_tabbar;
}

void termit_toggle_search()
{
#ifdef TERMIT_ENABLE_SEARCH
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(termit.b_toggle_search))) {
        gtk_widget_show(termit.search_entry);
        gtk_widget_show(termit.b_find_prev);
        gtk_widget_show(termit.b_find_next);
        gtk_window_set_focus(GTK_WINDOW(termit.main_window), termit.search_entry);
    } else {
        gtk_widget_hide(termit.search_entry);
        gtk_widget_hide(termit.b_find_prev);
        gtk_widget_hide(termit.b_find_next);
        gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
        TERMIT_GET_TAB_BY_INDEX(pTab, page);
        gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);
    }
#endif // TERMIT_ENABLE_SEARCH
}

void termit_after_show_all()
{
    termit_set_fonts();
    termit_hide_scrollbars();
    termit_set_colors();
    termit_toggle_menubar();
    termit_check_tabbar();
    termit_toggle_search();
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
    termit_after_show_all();
}

void termit_set_statusbar_message(guint page)
{
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    TRACE("%s page=%d get_statusbar_callback=%d", __FUNCTION__, page, configs.get_statusbar_callback);
    if (configs.get_statusbar_callback) {
        gchar* statusbarMessage = termit_lua_getStatusbarCallback(configs.get_statusbar_callback, page);
        TRACE("statusbarMessage=[%s]", statusbarMessage);
        gtk_statusbar_push(GTK_STATUSBAR(termit.statusbar), 0, statusbarMessage);
        g_free(statusbarMessage);
    } else {
        gtk_statusbar_push(GTK_STATUSBAR(termit.statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
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

    termit_check_tabbar();
}

static void termit_tab_add_matches(struct TermitTab* pTab, GArray* matches)
{
    guint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        struct Match tabMatch = {};
        tabMatch.lua_callback = match->lua_callback;
        tabMatch.pattern = match->pattern;
        tabMatch.tag = vte_terminal_match_add_gregex(VTE_TERMINAL(pTab->vte), match->regex, match->flags);
        vte_terminal_match_set_cursor_type(VTE_TERMINAL(pTab->vte), tabMatch.tag, GDK_HAND2);
        g_array_append_val(pTab->matches, tabMatch);
    }
}

#ifdef TERMIT_ENABLE_SEARCH
void termit_search_find_next()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    TRACE("%s tab=%p page=%d", __FUNCTION__, pTab, page);
    vte_terminal_search_find_next(VTE_TERMINAL(pTab->vte));
}

void termit_search_find_prev()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    TRACE("%s tab=%p page=%d", __FUNCTION__, pTab, page);
    vte_terminal_search_find_previous(VTE_TERMINAL(pTab->vte));
}
#endif // TERMIT_ENABLE_SEARCH

void termit_tab_set_transparency(struct TermitTab* pTab, gdouble transparency)
{
    pTab->style.transparency = transparency;
    if (transparency) {
        if (pTab->style.image_file == NULL) {
            vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        } else {
            vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
        }
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), pTab->style.transparency);
    } else {
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), pTab->style.transparency);
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
    }
}

static void termit_for_each_row_execute(struct TermitTab* pTab, glong row_start, glong row_end, int lua_callback)
{
    glong i = row_start;
    for (; i < row_end; ++i) {
        char* str = vte_terminal_get_text_range(VTE_TERMINAL(pTab->vte), i, 0, i, 500, NULL, &lua_callback, NULL);
        str[strlen(str) - 1] = '\0';
        termit_lua_dofunction2(lua_callback, str);
        free(str);
    }
}

void termit_for_each_row(int lua_callback)
{
    TRACE("%s lua_callback=%d", __FUNCTION__, lua_callback);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(pTab->scrollbar));
    const glong rows_total = gtk_adjustment_get_upper(adj);
    termit_for_each_row_execute(pTab, 0, rows_total, lua_callback);
}

void termit_for_each_visible_row(int lua_callback)
{
    TRACE("%s lua_callback=%d", __FUNCTION__, lua_callback);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(pTab->scrollbar));
    const glong row_start = ceil(gtk_adjustment_get_value(adj));
    const glong page_size = vte_terminal_get_row_count(VTE_TERMINAL(pTab->vte));
    termit_for_each_row_execute(pTab, row_start, row_start + page_size, lua_callback);
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

void termit_tab_set_pos(struct TermitTab* pTab, int newPos)
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    gint pagesCnt = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    if (newPos < 0) {
        newPos = pagesCnt - 1;
    } else if (newPos >= pagesCnt) {
        newPos = 0;
    }
    gtk_notebook_reorder_child(GTK_NOTEBOOK(termit.notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(termit.notebook), index), newPos);
}

void termit_append_tab_with_details(const struct TabInfo* ti)
{
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
    if (configs.tab_pos == GTK_POS_RIGHT) {
        gtk_label_set_angle(GTK_LABEL(pTab->tab_name), 270);
    } else if (configs.tab_pos == GTK_POS_LEFT) {
        gtk_label_set_angle(GTK_LABEL(pTab->tab_name), 90);
    }
    pTab->encoding = (ti->encoding) ? g_strdup(ti->encoding) : g_strdup(configs.default_encoding);
    pTab->bksp_binding = ti->bksp_binding;
    pTab->delete_binding = ti->delete_binding;
    pTab->hbox = gtk_hbox_new(FALSE, 0);
    pTab->vte = vte_terminal_new();

    vte_terminal_set_size(VTE_TERMINAL(pTab->vte), configs.cols, configs.rows);
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(pTab->vte), configs.scrollback_lines);
    if (configs.default_word_chars)
        vte_terminal_set_word_chars(VTE_TERMINAL(pTab->vte), configs.default_word_chars);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(pTab->vte), TRUE);
    vte_terminal_set_backspace_binding(VTE_TERMINAL(pTab->vte), pTab->bksp_binding);
    vte_terminal_set_delete_binding(VTE_TERMINAL(pTab->vte), pTab->delete_binding);
#ifdef TERMIT_ENABLE_SEARCH
    vte_terminal_search_set_wrap_around(VTE_TERMINAL(pTab->vte), TRUE);
#endif // TERMIT_ENABLE_SEARCH

    /* parse command */
    gchar **cmd_argv;
    GError *cmd_err = NULL;
    gchar *cmd_path = NULL;
    gchar *cmd_file = NULL;

    pTab->command = (ti->command) ? g_strdup(ti->command) : g_strdup(configs.default_command);
    if (!g_shell_parse_argv(pTab->command, NULL, &cmd_argv, &cmd_err)) {
        ERROR("%s", _("Cannot parse command. Creating tab with shell"));
        g_error_free(cmd_err);
    } else {
        cmd_path = g_find_program_in_path(cmd_argv[0]);
        cmd_file = g_path_get_basename(cmd_argv[0]);
    }

    TRACE("command=%s cmd_path=%s cmd_file=%s", pTab->command, cmd_path, cmd_file);
    if (cmd_path && cmd_file) {
        g_free(cmd_argv[0]);
        cmd_argv[0] = g_strdup(cmd_path);
#if VTE_CHECK_VERSION(0, 26, 0) > 0
        if (vte_terminal_fork_command_full(VTE_TERMINAL(pTab->vte),
                VTE_PTY_DEFAULT,
                ti->working_dir, 
                cmd_argv, NULL,
                0,
                NULL, NULL,
                &pTab->pid,
                &cmd_err) != TRUE) {
            ERROR("failed to open tab: %s", cmd_err->message);
            g_error_free(cmd_err);
        }
#else
        pTab->pid = vte_terminal_fork_command(VTE_TERMINAL(pTab->vte),
                cmd_path, cmd_argv, NULL, ti->working_dir, TRUE, TRUE, TRUE);
#endif // version >= 0.26
    } else {
        g_free(pTab->command);
        pTab->command = g_strdup(configs.default_command);
        gchar* argv[] = {pTab->command, NULL};
        TRACE("defaults: cmd=%s working_dir=%s", pTab->command, ti->working_dir);
        /* default tab */
#if VTE_CHECK_VERSION(0, 26, 0) > 0
        if (vte_terminal_fork_command_full(VTE_TERMINAL(pTab->vte),
                    VTE_PTY_DEFAULT,
                    ti->working_dir,
                    argv, NULL,
                    G_SPAWN_SEARCH_PATH,
                    NULL, NULL,
                    &pTab->pid,
                    &cmd_err) != TRUE) {
            ERROR("failed to open tab: %s", cmd_err->message);
            g_error_free(cmd_err);
        }
#else
        pTab->pid = vte_terminal_fork_command(VTE_TERMINAL(pTab->vte),
                pTab->command, NULL, NULL, ti->working_dir, TRUE, TRUE, TRUE);
#endif // version >= 0.26
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
    if (configs.fill_tabbar) {
        GValue val = {};
        g_value_init(&val, G_TYPE_BOOLEAN);
        g_value_set_boolean(&val, TRUE);
        gtk_container_child_set_property(GTK_CONTAINER(termit.notebook), pTab->hbox, "tab-expand", &val);
        gtk_container_child_set_property(GTK_CONTAINER(termit.notebook), pTab->hbox, "tab-fill", &val);
    }

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
    g_object_set_data(G_OBJECT(tabWidget), TERMIT_TAB_DATA, pTab);

    if (index == 0) { // there is no "switch-page" signal on the first page
        termit_set_statusbar_message(index);
    }
    pTab->scrollbar_is_shown = configs.show_scrollbar;
    gtk_widget_show_all(termit.notebook);

    if (pTab->style.image_file == NULL) {
        vte_terminal_set_background_image(VTE_TERMINAL(pTab->vte), NULL);
    } else {
        vte_terminal_set_background_image_file(VTE_TERMINAL(pTab->vte), pTab->style.image_file);
    }
    termit_tab_apply_colors(pTab);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE);
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);

    termit_check_tabbar();
    termit_hide_scrollbars();
}

void termit_append_tab_with_command(const gchar* command)
{
    struct TabInfo ti = {};
    ti.bksp_binding = configs.default_bksp;
    ti.delete_binding = configs.default_delete;
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
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page);
    TRACE("%s tab=%p page=%d encoding=%s", __FUNCTION__, pTab, page, encoding);
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), encoding);
    g_free(pTab->encoding);
    pTab->encoding = g_strdup(encoding);
    termit_set_statusbar_message(page);
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

void termit_tab_set_font_by_index(gint tab_index, const gchar* font_name)
{
    TRACE("%s: tab_index=%d font=%s", __FUNCTION__, tab_index, font_name);
    if (tab_index < 0) {
        tab_index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    }
    TERMIT_GET_TAB_BY_INDEX(pTab, tab_index);
    termit_tab_set_font(pTab, font_name);
}

void termit_tab_set_background_image(struct TermitTab* pTab, const gchar* image_file)
{
    TRACE("pTab->image_file=%s image_file=%s", pTab->style.image_file, image_file);
    if (pTab->style.image_file) {
        g_free(pTab->style.image_file);
    }
    if (image_file == NULL) {
        pTab->style.image_file = NULL;
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        vte_terminal_set_background_image(VTE_TERMINAL(pTab->vte), NULL);
    } else {
        pTab->style.image_file = g_strdup(image_file);
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
        vte_terminal_set_background_image_file(VTE_TERMINAL(pTab->vte), pTab->style.image_file);
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

void termit_tab_set_color_foreground_by_index(gint tab_index, const GdkColor* p_color)
{
    termit_set_color__(tab_index, p_color, termit_tab_set_color_foreground);
}

void termit_tab_set_color_background_by_index(gint tab_index, const GdkColor* p_color)
{
    termit_set_color__(tab_index, p_color, termit_tab_set_color_background);
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

static void clipboard_received_text(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
    if (text) {
        gchar** d = (gchar**)data;
        *d = g_strdup(text);
    }
}

gchar* termit_get_selection()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX2(pTab, page, NULL);
    if (vte_terminal_get_has_selection(VTE_TERMINAL(pTab->vte)) == FALSE) {
        return NULL;
    }
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gchar* text = NULL;
    gtk_clipboard_request_text(clip, clipboard_received_text, &text);
    if (!text)
        return NULL;
    return text;
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

void termit_set_show_scrollbar_signal(GtkWidget* menuItem, gpointer pHandlerId)
{
    gulong handlerId = g_signal_connect(G_OBJECT(menuItem), "toggled",
            G_CALLBACK(termit_on_toggle_scrollbar), NULL);
    if (pHandlerId == NULL) {
        pHandlerId = g_malloc(sizeof(handlerId));
    }
    memcpy(pHandlerId, &handlerId, sizeof(handlerId));
    g_object_set_data(G_OBJECT(menuItem), "handlerId", pHandlerId);
}

