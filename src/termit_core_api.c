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
        TERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        if (!pTab->scrollbar_is_shown) {
            gtk_widget_hide(pTab->scrollbar);
        }
    }
}

void termit_tab_set_color_foreground(struct TermitTab* pTab, const GdkRGBA* p_color)
{
    if (p_color) {
        pTab->style.foreground_color = gdk_rgba_copy(p_color);
        vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        if (pTab->style.foreground_color) {
            vte_terminal_set_color_bold(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        }
    }
}

void termit_tab_set_color_background(struct TermitTab* pTab, const GdkRGBA* p_color)
{
    if (p_color) {
        pTab->style.background_color = gdk_rgba_copy(p_color);
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
        TERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        termit_tab_apply_colors(pTab);
    }
}

static void termit_set_fonts()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    GtkWidget* widget = GTK_WIDGET(termit.main_window);
    GtkBorder padding = {};
    gtk_style_context_get_padding(gtk_widget_get_style_context(widget),
            gtk_widget_get_state_flags(widget),
            &padding);
    gint charWidth = 0, charHeight = 0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        vte_terminal_set_font(VTE_TERMINAL(pTab->vte), pTab->style.font);
        const gint cw = vte_terminal_get_char_width(VTE_TERMINAL(pTab->vte));
        const gint ch = vte_terminal_get_char_height(VTE_TERMINAL(pTab->vte));
        charWidth = charWidth > cw ? charWidth : cw;
        charHeight = charHeight > ch ? charHeight : ch;
    }
    const gint newWidth = charWidth * configs.cols + padding.left + padding.right;
    const gint newHeight = charHeight * configs.rows + padding.top + padding.bottom;
    gint oldWidth, oldHeight;
    gtk_window_get_size(GTK_WINDOW(termit.main_window), &oldWidth, &oldHeight);
    const gint width = (newWidth > oldWidth) ? newWidth : oldWidth;
    const gint height = (newHeight > oldHeight) ? newHeight : oldHeight;
    gtk_window_resize(GTK_WINDOW(termit.main_window), width, height);

    GdkGeometry geom;
    geom.min_width = width;
    geom.min_height = height;
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
        TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
        gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);
    }
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
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
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

void termit_del_tab_n(gint page)
{
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s page=%d pid=%d", __FUNCTION__, page, pTab->pid);
    g_signal_handler_disconnect(G_OBJECT(pTab->vte), pTab->onChildExitedHandlerId);
    g_array_free(pTab->matches, TRUE);
    g_free(pTab->encoding);
    g_strfreev(pTab->argv);
    g_free(pTab->title);
    termit_style_free(&pTab->style);
    g_free(pTab->search_regex);
    g_free(pTab);
    gtk_notebook_remove_page(GTK_NOTEBOOK(termit.notebook), page);

    termit_check_tabbar();
}

static void termit_del_tab()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    termit_del_tab_n(page);
}

static void termit_tab_add_matches(struct TermitTab* pTab, GArray* matches)
{
    guint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        struct Match tabMatch = {};
        tabMatch.lua_callback = match->lua_callback;
        tabMatch.pattern = match->pattern;
        tabMatch.tag = vte_terminal_match_add_regex(VTE_TERMINAL(pTab->vte), match->regex, match->flags);
        vte_terminal_match_set_cursor_type(VTE_TERMINAL(pTab->vte), tabMatch.tag, GDK_HAND2);
        g_array_append_val(pTab->matches, tabMatch);
    }
}

void termit_search_find_next()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s tab=%p page=%d", __FUNCTION__, pTab, page);
    vte_terminal_search_find_next(VTE_TERMINAL(pTab->vte));
}

void termit_search_find_prev()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s tab=%p page=%d", __FUNCTION__, pTab, page);
    vte_terminal_search_find_previous(VTE_TERMINAL(pTab->vte));
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
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(pTab->scrollbar));
    const glong rows_total = gtk_adjustment_get_upper(adj);
    termit_for_each_row_execute(pTab, 0, rows_total, lua_callback);
}

void termit_for_each_visible_row(int lua_callback)
{
    TRACE("%s lua_callback=%d", __FUNCTION__, lua_callback);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
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
    pTab->cursor_blink_mode = ti->cursor_blink_mode;
    pTab->cursor_shape = ti->cursor_shape;
    pTab->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    pTab->vte = vte_terminal_new();
    VteTerminal* vte = VTE_TERMINAL(pTab->vte);

    vte_terminal_set_size(vte, configs.cols, configs.rows);
    vte_terminal_set_scrollback_lines(vte, configs.scrollback_lines);
    vte_terminal_set_scroll_on_output(vte, configs.scroll_on_output);
    vte_terminal_set_scroll_on_keystroke(vte, configs.scroll_on_keystroke);
    if (configs.default_word_char_exceptions) {
        vte_terminal_set_word_char_exceptions(vte, configs.default_word_char_exceptions);
    }
    vte_terminal_set_mouse_autohide(vte, TRUE);
    vte_terminal_set_backspace_binding(vte, pTab->bksp_binding);
    vte_terminal_set_delete_binding(vte, pTab->delete_binding);
    vte_terminal_set_cursor_blink_mode(vte, pTab->cursor_blink_mode);
    vte_terminal_set_cursor_shape(vte, pTab->cursor_shape);
    vte_terminal_search_set_wrap_around(vte, TRUE);

    guint l = 0;
    if (ti->argv == NULL) {
        l = 1;
        pTab->argv = (gchar**)g_new0(gchar*, 2);
        pTab->argv[0] = g_strdup(configs.default_command);
    } else {
        while (ti->argv[l] != NULL) {
            ++l;
        }
        pTab->argv = (gchar**)g_new0(gchar*, l + 1);
        guint i = 0;
        for (; i < l; ++i) {
            pTab->argv[i] = g_strdup(ti->argv[i]);
        }
    }
    g_assert(l >= 1);
    /* parse command */
    GError *cmd_err = NULL;
    if (l == 1) { // arguments may be in one compound string
        gchar **cmd_argv;
        if (!g_shell_parse_argv(pTab->argv[0], NULL, &cmd_argv, &cmd_err)) {
            ERROR("%s", _("Cannot parse command. Creating tab with shell"));
            g_error_free(cmd_err);
            return;
        }
        g_strfreev(pTab->argv);
        pTab->argv = cmd_argv;
    }
    gchar *cmd_path = NULL;
    cmd_path = g_find_program_in_path(pTab->argv[0]);

    if (cmd_path != NULL) {
        g_free(pTab->argv[0]);
        pTab->argv[0] = g_strdup(cmd_path);
        g_free(cmd_path);
    }
    if (vte_terminal_spawn_sync(vte,
            VTE_PTY_DEFAULT,
            ti->working_dir,
            pTab->argv, NULL,
            0, NULL, NULL,
            &pTab->pid,
            NULL, // g_cancellable_new
            &cmd_err) != TRUE) {
        ERROR("failed to open tab: %s", cmd_err->message);
        g_error_free(cmd_err);
        return;
    }
    TRACE("command=%s pid=%d", pTab->argv[0], pTab->pid);

    g_signal_connect(G_OBJECT(pTab->vte), "bell", G_CALLBACK(termit_on_beep), pTab);
    g_signal_connect(G_OBJECT(pTab->vte), "focus-in-event", G_CALLBACK(termit_on_focus), pTab);
    g_signal_connect(G_OBJECT(pTab->vte), "window-title-changed", G_CALLBACK(termit_on_tab_title_changed), NULL);

    pTab->onChildExitedHandlerId = g_signal_connect(G_OBJECT(pTab->vte), "child-exited", G_CALLBACK(termit_on_child_exited), &termit_close_tab);
    g_signal_connect_swapped(G_OBJECT(pTab->vte), "button-press-event", G_CALLBACK(termit_on_popup), NULL);

    if (vte_terminal_set_encoding(vte, pTab->encoding, &cmd_err) != TRUE) {
        ERROR("cannot set encoding (%s): %s", pTab->encoding, cmd_err->message);
        g_error_free(cmd_err);
        return;
    }

    pTab->matches = g_array_new(FALSE, TRUE, sizeof(struct Match));
    termit_tab_add_matches(pTab, configs.matches);
    vte_terminal_set_font(vte, pTab->style.font);

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

    pTab->scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL,
        gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(pTab->vte)));

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

    termit_tab_apply_colors(pTab);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(termit.notebook), index);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(termit.notebook), pTab->hbox, TRUE);
    gtk_window_set_focus(GTK_WINDOW(termit.main_window), pTab->vte);

    termit_check_tabbar();
    termit_hide_scrollbars();
}

void termit_append_tab_with_command(gchar** argv)
{
    struct TabInfo ti = {};
    ti.bksp_binding = configs.default_bksp;
    ti.delete_binding = configs.default_delete;
    ti.cursor_blink_mode = configs.default_blink;
    ti.cursor_shape = configs.default_shape;
    ti.argv = argv;
    termit_append_tab_with_details(&ti);
}

void termit_append_tab()
{
    termit_append_tab_with_command(NULL);
}

void termit_set_encoding(const gchar* encoding)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s tab=%p page=%d encoding=%s", __FUNCTION__, pTab, page, encoding);
    if (vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), encoding, NULL) != TRUE) {
        ERROR("Cannot set encoding %s", encoding);
    } else {
        g_free(pTab->encoding);
        pTab->encoding = g_strdup(encoding);
        termit_set_statusbar_message(page);
    }
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
    if (pTab->title) {
        g_free(pTab->title);
    }
    pTab->title = tmp_title;
    gtk_label_set_text(GTK_LABEL(pTab->tab_name), pTab->title);
    termit_set_window_title(title);
}

void termit_set_default_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
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
    TERMIT_GET_TAB_BY_INDEX(pTab, tab_index, return);
    termit_tab_set_font(pTab, font_name);
}

static void termit_set_color__(gint tab_index, const GdkRGBA* p_color, void (*callback)(struct TermitTab*, const GdkRGBA*))
{
    TRACE("%s: tab_index=%d color=%p", __FUNCTION__, tab_index, p_color);
    if (!p_color) {
        TRACE_MSG("p_color is NULL");
        return;
    }
    if (tab_index < 0) {
        tab_index = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    }
    TERMIT_GET_TAB_BY_INDEX(pTab, tab_index, return);
    callback(pTab, p_color);
}

void termit_tab_set_color_foreground_by_index(gint tab_index, const GdkRGBA* p_color)
{
    termit_set_color__(tab_index, p_color, termit_tab_set_color_foreground);
}

void termit_tab_set_color_background_by_index(gint tab_index, const GdkRGBA* p_color)
{
    termit_set_color__(tab_index, p_color, termit_tab_set_color_background);
}

void termit_paste()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    vte_terminal_paste_clipboard(VTE_TERMINAL(pTab->vte));
}

void termit_copy()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(termit.notebook));
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    vte_terminal_copy_clipboard_format(VTE_TERMINAL(pTab->vte), VTE_FORMAT_TEXT);
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
    TERMIT_GET_TAB_BY_INDEX(pTab, page, return NULL);
    if (vte_terminal_get_has_selection(VTE_TERMINAL(pTab->vte)) == FALSE) {
        return NULL;
    }
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gchar* text = NULL;
    gtk_clipboard_request_text(clip, clipboard_received_text, &text);
    if (!text) {
        return NULL;
    }
    return text;
}

void termit_close_tab()
{
    termit_del_tab();
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) == 0) {
        TRACE("no pages left, exiting");
        termit_quit();
    }
}

void termit_activate_tab(gint tab_index)
{
    if (tab_index < 0) {
        TRACE("tab_index(%d) < 0: skipping", tab_index);
        return;
    }
    if (tab_index >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook))) {
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
    while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)) > 0) {
        termit_del_tab();
    }
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
    if (!title) {
        return;
    }
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
