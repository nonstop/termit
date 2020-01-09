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

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>

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
        ERROR("tabWidget not found by index=%u", index);
        return NULL;
    }
    struct TermitTab* pTab = (struct TermitTab*)g_object_get_data(G_OBJECT(tabWidget), TERMIT_TAB_DATA);
    return pTab;
}

struct TermitTab* termit_get_tab_by_vte(VteTerminal* vte, gint* page)
{
    gint sz = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    for (gint p = 0; p < sz; ++p) {
        struct TermitTab* pTab = termit_get_tab_by_index(p);
        if (pTab && VTE_TERMINAL(pTab->vte) == vte) {
            *page = p;
            return pTab;
        }
    }
    ERROR("tabWidget not found by vte=%p", vte);
    return NULL;
}

static GtkWidget* create_statusbar()
{
    GtkWidget* statusbar = gtk_statusbar_new();
    return statusbar;
}

static void create_search(struct TermitData* termit)
{
    termit->b_toggle_search = gtk_toggle_button_new();
    gtk_button_set_image(GTK_BUTTON(termit->b_toggle_search),
            gtk_image_new_from_icon_name("edit-find", GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(termit->b_toggle_search), "toggled", G_CALLBACK(termit_on_toggle_search), NULL);

    termit->b_find_next = gtk_button_new_from_icon_name("go-next", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(G_OBJECT(termit->b_find_next), "clicked", G_CALLBACK(termit_on_find_next), NULL);

    termit->b_find_prev = gtk_button_new_from_icon_name("go-previous", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(G_OBJECT(termit->b_find_prev), "clicked", G_CALLBACK(termit_on_find_prev), NULL);

    termit->search_entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(termit->search_entry), "key-press-event", G_CALLBACK(termit_on_search_keypress), NULL);
}

static void pack_widgets()
{
    termit.hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.menu_bar, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.b_toggle_search, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.search_entry, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.b_find_prev, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.b_find_next, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.statusbar, TRUE, 1, 0);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), termit.notebook, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), termit.hbox, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(termit.main_window), vbox);
    if (!gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook))) {
        termit_append_tab();
    }
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
    GtkWidget *mi_new_tab = termit_lua_menu_item_from_string(_("Open"), "openTab");
    GtkWidget *mi_close_tab = termit_lua_menu_item_from_string(_("Delete"), "closeTab");
    GtkWidget *mi_exit = termit_lua_menu_item_from_string(_("Quit"), "quit");

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
    GtkWidget *mi_edit_preferences = termit_lua_menu_item_from_string(_("Preferences"), "preferencesDlg");
    GtkWidget *mi_copy = termit_lua_menu_item_from_string(_("Copy"), "copy");
    GtkWidget *mi_paste = termit_lua_menu_item_from_string(_("Paste"), "paste");

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
    GtkWidget *mi_load_session = termit_lua_menu_item_from_string(_("Open session"), "loadSessionDlg");
    GtkWidget *mi_save_session = termit_lua_menu_item_from_string(_("Save session"), "saveSessionDlg");

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

    GtkWidget *mi_new_tab = termit_lua_menu_item_from_string(_("Open"), "openTab");
    GtkWidget *mi_close_tab = termit_lua_menu_item_from_string(_("Delete"), "closeTab");
    GtkWidget *mi_set_tab_name = termit_lua_menu_item_from_string(_("Set tab name..."), "setTabTitleDlg");
    GtkWidget *mi_edit_preferences = termit_lua_menu_item_from_string(_("Preferences"), "preferencesDlg");
    GtkWidget *mi_copy = termit_lua_menu_item_from_string(_("Copy"), "copy");
    GtkWidget *mi_paste = termit_lua_menu_item_from_string(_("Paste"), "paste");
    GtkWidget *mi_exit = termit_lua_menu_item_from_string(_("Quit"), "quit");
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
    TRACE("user_popup_menus->len=%d", configs.user_popup_menus->len);
    guint j = 0;
    for (; j<configs.user_popup_menus->len; ++j) {
        struct UserMenu* um = &g_array_index(configs.user_popup_menus, struct UserMenu, j);

        GtkWidget *mi_util = gtk_menu_item_new_with_label(um->name);
        GtkWidget *utils_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_util), utils_menu);

        TRACE("%s items->len=%d", um->name, um->items->len);
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

static void termit_init(const gchar* initFile, gchar** argv)
{
    termit_init_sessions();
    termit_configs_set_defaults();

    termit.tab_max_number = 1;

    termit.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_icon_name("utilities-terminal");
    termit.statusbar = create_statusbar();
    create_search(&termit);
    termit.notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(termit.notebook), TRUE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);

    termit_lua_init(initFile);

    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(termit.notebook), configs.tab_pos);

    if (argv) {
        termit_append_tab_with_command(argv);
    }

    termit_create_menubar();
    pack_widgets();
    termit_create_popup_menu();

    if (!configs.allow_changing_title) {
        termit_set_window_title(configs.default_window_title);
    }
    if (configs.hide_titlebar_when_maximized) {
        gtk_window_set_hide_titlebar_when_maximized(GTK_WINDOW(termit.main_window), TRUE);
    }
    if (configs.start_maximized) {
        gtk_window_maximize(GTK_WINDOW(termit.main_window));
    }
    gtk_notebook_set_show_border(GTK_NOTEBOOK(termit.notebook), configs.show_border);
}

enum {
    TERMIT_GETOPT_HELP = 'h',
    TERMIT_GETOPT_VERSION = 'v',
    TERMIT_GETOPT_INIT = 'i',
    TERMIT_GETOPT_NAME = 'n',
    TERMIT_GETOPT_CLASS = 'c',
    TERMIT_GETOPT_ROLE = 'r',
    TERMIT_GETOPT_TITLE = 'T'
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
"  -T, --title=title      - set window title\n"
"", PACKAGE_VERSION);
}
/*
    Support the command-line option "-e <command>", which creates a new
    terminal window and runs the specified command.  <command> may be
    multiple arguments, which form the argument list to the executed
    program.  In other words, the behavior is as though the arguments
    were passed directly to execvp, bypassing the shell.  (xterm's
    behavior of falling back on using the shell if -e had a single
    argument and exec failed is permissible but not required.)

    exec($T, "-e", "bc")              --> runs bc
    exec($T, "-e", "bc --quiet")      --> fails OR runs bc --quiet [1]
    exec($T, "-e", "bc", "--quiet")   --> runs bc --quiet
    exec($T, "-e", "bc *")            --> fails OR runs bc * (wildcard) [1]
    exec($T, "-e", "bc", "*")         --> runs bc * (literal)

[1] Programs are allowed to pass the entire argument to -e to the shell
    if and only if there is only one argument and exec fails.
*/

static GArray* parse_execute_args(int argc, char **argv)
{
    GArray* arr = NULL;
    int i = 0;
    int foundE = 0;
    for (; i < argc; ++i) {
        if (foundE) {
            gchar* arg = g_strdup(argv[i]);
            g_array_append_val(arr, arg);
        } else {
            TRACE("[%s] %d", argv[i], strncmp(argv[i], "--execute=", 10));
            if ((strcmp(argv[i], "-e") == 0)
                    || (strcmp(argv[i], "--execute") == 0)) {
                foundE = 1;
                arr = g_array_new(FALSE, TRUE, sizeof(gchar*));
            } else if (strncmp(argv[i], "--execute=", 10) == 0) {
                foundE = 1;
                TRACE("arg=[%s]", argv[i] + 10);
                arr = g_array_new(FALSE, TRUE, sizeof(gchar*));
                gchar* arg = g_strdup(argv[i] + 10);
                g_array_append_val(arr, arg);
            }
        }
    }
    if (arr == NULL) {
        return NULL;
    }
    if (arr->len > 0) {
        return arr;
    }
    g_array_free(arr, TRUE);
    return NULL;
}

int main(int argc, char **argv)
{
    opterr = 0; // suppress "invalid option" error message from getopt
    gchar* initFile = NULL;
    GArray* arrArgv = parse_execute_args(argc, argv);;
    gchar *windowName = NULL, *windowClass = NULL, *windowRole = NULL, *windowTitle = NULL;
    while (1) {
        static struct option long_options[] = {
            {"help", no_argument, 0, TERMIT_GETOPT_HELP},
            {"version", no_argument, 0, TERMIT_GETOPT_VERSION},
            {"init", required_argument, 0, TERMIT_GETOPT_INIT},
            {"name", required_argument, 0, TERMIT_GETOPT_NAME},
            {"class", required_argument, 0, TERMIT_GETOPT_CLASS},
            {"role", required_argument, 0, TERMIT_GETOPT_ROLE},
            {"title", required_argument, 0, TERMIT_GETOPT_TITLE},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int flag = getopt_long(argc, argv, "-hvi:n:c:r:T:", long_options, &option_index);
        /* Detect the end of the options. */
        if (flag == -1) {
            break;
        }

        switch (flag) {
        case TERMIT_GETOPT_HELP:
            termit_print_usage();
            return 0;
        case TERMIT_GETOPT_VERSION:
            g_printf(PACKAGE_VERSION);
            g_printf("\n");
            return 0;
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
        case TERMIT_GETOPT_TITLE:
            windowTitle = g_strdup(optarg);
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
    gchar** cmdArgv = NULL;
    if (arrArgv != NULL) {
        cmdArgv = (gchar**)g_new0(gchar*, arrArgv->len + 1);
        guint i = 0;
        for (; i < arrArgv->len; ++i) {
            cmdArgv[i] = g_array_index(arrArgv, gchar*, i);
            TRACE("    %d=[%s]", i, cmdArgv[i]);
        }
        g_array_free(arrArgv, TRUE);
    }
    termit_init(initFile, cmdArgv);
    g_strfreev(cmdArgv);
    g_free(initFile);

    g_signal_connect(G_OBJECT (termit.main_window), "delete_event", G_CALLBACK (termit_on_delete_event), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "destroy", G_CALLBACK (termit_on_destroy), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "key-press-event", G_CALLBACK(termit_on_key_press), NULL);

    TRACE("window: name=[%s] class=[%s] role=[%s] title=[%s]",
            windowName, windowClass, windowRole, windowTitle);
    if (windowName || windowClass) {
        gtk_window_set_wmclass(GTK_WINDOW(termit.main_window), windowName, windowClass);
        g_free(windowName);
        g_free(windowClass);
    }
    if (windowRole) {
        gtk_window_set_role(GTK_WINDOW(termit.main_window), windowRole);
        g_free(windowRole);
    }
    if (windowTitle) {
        configs.allow_changing_title = FALSE;
        gtk_window_set_title(GTK_WINDOW(termit.main_window), windowTitle);
        g_free(windowTitle);
    }
    // Disable menubar on F10
    g_object_set(G_OBJECT(gtk_widget_get_settings(termit.main_window)), "gtk-menu-bar-accel", "", NULL);
    /* Show the application window */
    gtk_widget_show_all(termit.main_window);

    // actions after display
    termit_after_show_all();

    gtk_main();
    termit_config_deinit();
    termit_lua_close();
    return 0;
}
