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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>
#include <stdlib.h>

#include <getopt.h>

#include "config.h"

#include "termit_core_api.h"
#include "lua_api.h"
#include "callbacks.h"
#include "sessions.h"
#include "keybindings.h"
#include "configs.h"
#include "termit.h"

struct TermitData termit = {};

struct TermitTab* termit_get_tab_by_index(guint index)
{
    GtkWidget* tabWidget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(termit.notebook), index);
    if (!tabWidget) {
        ERROR("tabWidget at %zd is NULL", index);
        return NULL;
    }
    struct TermitTab* pTab = (struct TermitTab*)g_object_get_data(G_OBJECT(tabWidget), TERMIT_TAB_DATA);
    return pTab;
}

static GtkWidget* create_statusbar()
{
    GtkWidget* statusbar = gtk_statusbar_new();
    return statusbar;
}

static void create_search(struct TermitData* termit)
{
#ifdef TERMIT_ENABLE_SEARCH
    termit->b_toggle_search = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(termit->b_toggle_search),
            gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(termit->b_toggle_search), "toggled", G_CALLBACK(termit_on_toggle_search), NULL);

    termit->b_find_next = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(termit->b_find_next),
            gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(termit->b_find_next), "clicked", G_CALLBACK(termit_on_find_next), NULL);

    termit->b_find_prev = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(termit->b_find_prev),
            gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(termit->b_find_prev), "clicked", G_CALLBACK(termit_on_find_prev), NULL);

    termit->search_entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(termit->search_entry), "key-press-event", G_CALLBACK(termit_on_search_keypress), NULL);
#endif // TERMIT_ENABLE_SEARCH
}

static void pack_widgets()
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    termit.hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.menu_bar, FALSE, 0, 0);
#ifdef TERMIT_ENABLE_SEARCH
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.b_toggle_search, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.search_entry, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.b_find_prev, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.b_find_next, FALSE, 0, 0);
#endif // TERMIT_ENABLE_SEARCH
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.statusbar, TRUE, 1, 0);

    gtk_box_pack_start(GTK_BOX(vbox), termit.notebook, TRUE, 1, 0);
    gtk_box_pack_start(GTK_BOX(vbox), termit.hbox, FALSE, 1, 0);
    gtk_container_add(GTK_CONTAINER(termit.main_window), vbox);

    if (!gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)))
        termit_append_tab();
    g_signal_connect(G_OBJECT(termit.notebook), "switch-page", G_CALLBACK(termit_on_switch_page), NULL);
    g_signal_connect(G_OBJECT(termit.main_window), "button-press-event", G_CALLBACK(termit_on_double_click), NULL);
}

static void menu_item_set_accel(GtkMenuItem* mi, const gchar* parentName,
        const gchar* itemName, const gchar* accel)
{
    struct KeyWithState kws = {};
    if (termit_parse_keys_str(accel, &kws) < 0) {
        ERROR("failed to parse keybinding: %s", accel);
        return;
    }
    gchar* path = NULL;
    path = g_strdup_printf("<Termit>/%s/%s", parentName, itemName);
    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(mi), path);
    gtk_accel_map_add_entry(path, kws.keyval, kws.state);
    g_free(path);
}

static void termit_create_menus(GtkWidget* menu_bar, GtkAccelGroup* accel, GArray* menus)
{
    TRACE("menus->len=%d", menus->len);
    guint j = 0;
    for (; j<menus->len; ++j) {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, j);

        GtkWidget *mi_menu = gtk_menu_item_new_with_label(um->name);
        GtkWidget *menu = gtk_menu_new();

        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_menu), menu);
        gtk_menu_set_accel_group(GTK_MENU(menu), accel);

        TRACE("%s items->len=%d", um->name, um->items->len);
        guint i = 0;
        gtk_menu_set_accel_group(GTK_MENU(menu), accel);
        for (; i<um->items->len; ++i) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, i);
            GtkWidget *mi_tmp = gtk_menu_item_new_with_label(umi->name);
            g_object_set_data(G_OBJECT(mi_tmp), TERMIT_USER_MENU_ITEM_DATA, umi);
            g_signal_connect(G_OBJECT(mi_tmp), "activate", G_CALLBACK(termit_on_menu_item_selected), NULL);
            if (umi->accel) {
                menu_item_set_accel(GTK_MENU_ITEM(mi_tmp), um->name, umi->name, umi->accel);
            }
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_tmp);
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mi_menu);
    }
}

static GtkWidget* termit_lua_menu_item_from_stock(const gchar* stock_id, const char* luaFunc)
{
    GtkWidget *mi = gtk_image_menu_item_new_from_stock(stock_id, NULL);
    g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(termit_on_menu_item_selected), NULL);
    struct UserMenuItem* umi = (struct UserMenuItem*)malloc(sizeof(struct UserMenuItem));
    umi->name = g_strdup(gtk_menu_item_get_label(GTK_MENU_ITEM(mi)));
    umi->accel = NULL;
    umi->lua_callback = termit_get_lua_func(luaFunc);
    g_object_set_data(G_OBJECT(mi), TERMIT_USER_MENU_ITEM_DATA, umi);
    return mi;
}

static GtkWidget* termit_lua_menu_item_from_string(const gchar* label, const char* luaFunc)
{
    GtkWidget *mi = gtk_menu_item_new_with_label(label);
    g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(termit_on_menu_item_selected), NULL);
    struct UserMenuItem* umi = (struct UserMenuItem*)malloc(sizeof(struct UserMenuItem));
    umi->name = g_strdup(gtk_menu_item_get_label(GTK_MENU_ITEM(mi)));
    umi->accel = NULL;
    umi->lua_callback = termit_get_lua_func(luaFunc);
    g_object_set_data(G_OBJECT(mi), TERMIT_USER_MENU_ITEM_DATA, umi);
    return mi;
}

void termit_create_menubar()
{

    GtkAccelGroup *accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(termit.main_window), accel);
    GtkWidget* menu_bar = gtk_menu_bar_new();

    // File menu
    GtkWidget *mi_new_tab = termit_lua_menu_item_from_stock(GTK_STOCK_ADD, "openTab");
    GtkWidget *mi_close_tab = termit_lua_menu_item_from_stock(GTK_STOCK_DELETE, "closeTab");
    GtkWidget *mi_exit = termit_lua_menu_item_from_stock(GTK_STOCK_QUIT, "quit");

    GtkWidget *mi_file = gtk_menu_item_new_with_label(_("File"));
    GtkWidget *file_menu = gtk_menu_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_new_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_close_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_exit);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_file), file_menu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mi_file);

    // Edit menu
    GtkWidget *mi_set_tab_name = termit_lua_menu_item_from_string(_("Set tab name..."), "setTabTitleDlg");
    GtkWidget *mi_edit_preferences = termit_lua_menu_item_from_stock(GTK_STOCK_PREFERENCES, "preferencesDlg");
    GtkWidget *mi_copy = termit_lua_menu_item_from_stock(GTK_STOCK_COPY, "copy");
    GtkWidget *mi_paste = termit_lua_menu_item_from_stock(GTK_STOCK_PASTE, "paste");

    GtkWidget *mi_edit = gtk_menu_item_new_with_label(_("Edit"));
    GtkWidget *edit_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_set_tab_name);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_edit_preferences);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_edit), edit_menu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mi_edit);

    // Sessions menu
    GtkWidget *mi_load_session = termit_lua_menu_item_from_stock(GTK_STOCK_OPEN, "loadSessionDlg");
    GtkWidget *mi_save_session = termit_lua_menu_item_from_stock(GTK_STOCK_SAVE, "saveSessionDlg");

    GtkWidget *mi_sessions = gtk_menu_item_new_with_label(_("Sessions"));
    GtkWidget *sessions_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(sessions_menu), mi_load_session);
    gtk_menu_shell_append(GTK_MENU_SHELL(sessions_menu), mi_save_session);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_sessions), sessions_menu);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mi_sessions);

    // User menus
    termit_create_menus(menu_bar, accel, configs.user_menus);

    termit.menu_bar = menu_bar;
}

void termit_create_popup_menu()
{
    termit.menu = gtk_menu_new();

    GtkWidget *mi_new_tab = termit_lua_menu_item_from_stock(GTK_STOCK_ADD, "openTab");
    GtkWidget *mi_close_tab = termit_lua_menu_item_from_stock(GTK_STOCK_DELETE, "closeTab");
    GtkWidget *mi_set_tab_name = termit_lua_menu_item_from_string(_("Set tab name..."), "setTabTitleDlg");
    GtkWidget *mi_edit_preferences = termit_lua_menu_item_from_stock(GTK_STOCK_PREFERENCES, "preferencesDlg");
    GtkWidget *mi_copy = termit_lua_menu_item_from_stock(GTK_STOCK_COPY, "copy");
    GtkWidget *mi_paste = termit_lua_menu_item_from_stock(GTK_STOCK_PASTE, "paste");
    GtkWidget *mi_exit = termit_lua_menu_item_from_stock(GTK_STOCK_QUIT, "quit");
    termit.mi_show_scrollbar = gtk_check_menu_item_new_with_label(_("Scrollbar"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(termit.mi_show_scrollbar), configs.show_scrollbar);
    termit_set_show_scrollbar_signal(termit.mi_show_scrollbar, NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_new_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_close_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_set_tab_name);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), termit.mi_show_scrollbar);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_edit_preferences);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_exit);

    // User popup menus
    TRACE("user_popup_menus->len=%zd", configs.user_popup_menus->len);
    guint j = 0;
    for (; j<configs.user_popup_menus->len; ++j) {
        struct UserMenu* um = &g_array_index(configs.user_popup_menus, struct UserMenu, j);

        GtkWidget *mi_util = gtk_menu_item_new_with_label(um->name);
        GtkWidget *utils_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_util), utils_menu);

        TRACE("%s items->len=%zd", um->name, um->items->len);
        guint i = 0;
        for (; i<um->items->len; i++) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, i);
            GtkWidget *mi_tmp = gtk_menu_item_new_with_label(umi->name);
            g_object_set_data(G_OBJECT(mi_tmp), TERMIT_USER_MENU_ITEM_DATA, umi);
            gtk_menu_shell_append(GTK_MENU_SHELL(utils_menu), mi_tmp);
            g_signal_connect(G_OBJECT(mi_tmp), "activate",
                G_CALLBACK(termit_on_menu_item_selected), NULL);
        }
        gtk_menu_shell_insert(GTK_MENU_SHELL(termit.menu), mi_util, 6);
    }
    gtk_widget_show_all(termit.menu);
}

static void termit_init(const gchar* initFile, const gchar* command)
{
    termit_init_sessions();
    termit_configs_set_defaults();

    termit.tab_max_number = 1;

    termit.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    termit.statusbar = create_statusbar();
    create_search(&termit);
    termit.notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);

    termit_lua_init(initFile);

    if (command) {
        TRACE("using command: %s", command);
        termit_append_tab_with_command(command);
    }

    termit_create_menubar();
    pack_widgets();
    termit_create_popup_menu();

    if (!configs.allow_changing_title)
        termit_set_window_title(configs.default_window_title);
}

enum {
    TERMIT_GETOPT_HELP = 'h',
    TERMIT_GETOPT_VERSION = 'v',
    TERMIT_GETOPT_EXEC = 'e',
    TERMIT_GETOPT_INIT = 'i',
    TERMIT_GETOPT_NAME = 'n',
    TERMIT_GETOPT_CLASS = 'c',
    TERMIT_GETOPT_ROLE = 'r'
};

static void termit_print_usage()
{
    g_print(
"termit %s - terminal emulator\n"
"\n"
"This program comes with NO WARRANTY, to the extent permitted by law.\n"
"You may redistribute copies of this program\n"
"under the terms of the GNU General Public License.\n"
"For more information about these matters, see the file named COPYING.\n"
"\n"
"Options:\n"
"  -h, --help             - print this help message\n"
"  -v, --version          - print version number\n"
"  -e, --execute          - execute command\n"
"  -i, --init=init_file   - use init_file instead of standard rc.lua\n"
"  -n, --name=name        - set window name hint\n"
"  -c, --class=class      - set window class hint\n"
"  -r, --role=role        - set window role (Gtk hint)\n"
"", PACKAGE_VERSION);
}

int main(int argc, char **argv)
{
    gchar* initFile = NULL;
    gchar* command = NULL;
    gchar *windowName = NULL, *windowClass = NULL, *windowRole = NULL;
    while (1) {
        static struct option long_options[] = {
            {"help", no_argument, 0, TERMIT_GETOPT_HELP},
            {"version", no_argument, 0, TERMIT_GETOPT_VERSION},
            {"execute", required_argument, 0, TERMIT_GETOPT_EXEC},
            {"init", required_argument, 0, TERMIT_GETOPT_INIT},
            {"name", required_argument, 0, TERMIT_GETOPT_NAME},
            {"class", required_argument, 0, TERMIT_GETOPT_CLASS},
            {"role", required_argument, 0, TERMIT_GETOPT_ROLE},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int flag = getopt_long(argc, argv, "hvi:e:n:c:r:", long_options, &option_index);

        /* Detect the end of the options. */
        if (flag == -1)
            break;

        switch (flag) {
        case TERMIT_GETOPT_HELP:
            termit_print_usage();
            return 0;
        case TERMIT_GETOPT_VERSION:
            g_printf(PACKAGE_VERSION);
            g_printf("\n");
            return 0;
        case TERMIT_GETOPT_EXEC:
            command = g_strdup(optarg);
            break;
        case TERMIT_GETOPT_INIT:
            initFile = g_strdup(optarg);
            break;
        case TERMIT_GETOPT_NAME:
            windowName = g_strdup(optarg);
            break;
        case TERMIT_GETOPT_CLASS:
            windowClass = g_strdup(optarg);
            break;
        case TERMIT_GETOPT_ROLE:
            windowRole = g_strdup(optarg);
            break;
        case '?':
            break;
        /* getopt_long already printed an error message. */
        default:
            break;
        }
    }

    bind_textdomain_codeset(PACKAGE, "UTF-8");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    gtk_init(&argc, &argv);
#ifdef __linux__
    signal(SIGCHLD, SIG_IGN);
#endif /* LINUX */
    termit_init(initFile, command);
    g_free(command);
    g_free(initFile);

    /**
     * dirty hack from gnome-terminal ;-)
     * F10 is used in many console apps, so we change global Gtk setting for termit
     * */
    gtk_settings_set_string_property(gtk_settings_get_default(), "gtk-menu-bar-accel",
        "<Shift><Control><Mod1><Mod2><Mod3><Mod4><Mod5>F10", "termit");

    g_signal_connect(G_OBJECT (termit.main_window), "delete_event", G_CALLBACK (termit_on_delete_event), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "destroy", G_CALLBACK (termit_on_destroy), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "key-press-event", G_CALLBACK(termit_on_key_press), NULL);

    TRACE("windowName=%s windowClass=%s windowRole=%s", windowName, windowClass, windowRole);
    if (windowName || windowClass) {
        gtk_window_set_wmclass(GTK_WINDOW(termit.main_window), windowName, windowClass);
        g_free(windowName);
        g_free(windowClass);
    }
    if (windowRole) {
        gtk_window_set_role(GTK_WINDOW(termit.main_window), windowRole);
        g_free(windowRole);
    }

    /* Show the application window */
    gtk_widget_show_all(termit.main_window);

    // actions after display
    termit_after_show_all();

    gtk_main();
    termit_config_deinit();
    termit_lua_close();
    return 0;
}

