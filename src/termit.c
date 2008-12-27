#include <gtk/gtk.h>
#include <vte/vte.h>
#include <string.h>

#include <getopt.h>

#include "config.h"

#include "termit_core_api.h"
#include "lua_api.h"
#include "callbacks.h"
#include "sessions.h"
#include "configs.h"
#include "termit.h"

struct TermitData termit = {0};

struct TermitTab* termit_get_tab_by_index(gint index)
{
    GtkWidget* tabWidget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(termit.notebook), index);
    if (!tabWidget) {
        ERROR("tabWidget is NULL");
        return NULL;
    }
    struct TermitTab* pTab = (struct TermitTab*)g_object_get_data(G_OBJECT(tabWidget), "termit.tab");
    return pTab;
}

static GtkWidget* create_statusbar()
{
    GtkWidget* statusbar = gtk_statusbar_new();
    return statusbar;
}

static void create_main_widgets(const gchar* command)
{
    termit.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    termit.statusbar = create_statusbar();
    termit.notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);
    
    if (command) {
        TRACE("using command: %s", command);
        termit_append_tab_with_command(command);
    }
}

static void pack_widgets()
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    termit.hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.menu_bar, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(termit.hbox), termit.statusbar, TRUE, 1, 0);

    gtk_box_pack_start(GTK_BOX(vbox), termit.notebook, TRUE, 1, 0);
    gtk_box_pack_start(GTK_BOX(vbox), termit.hbox, FALSE, 1, 0);
    gtk_container_add(GTK_CONTAINER(termit.main_window), vbox);
    
    if (!gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)))
        termit_append_tab();
    g_signal_connect(G_OBJECT(termit.notebook), "switch-page", G_CALLBACK(termit_on_switch_page), NULL);
    g_signal_connect(G_OBJECT(termit.main_window), "button-press-event", G_CALLBACK(termit_on_double_click), NULL);
}

void termit_create_menubar()
{
    GtkWidget* menu_bar = gtk_menu_bar_new();

    // File menu
    GtkWidget *mi_new_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
    g_signal_connect(G_OBJECT(mi_new_tab), "activate", G_CALLBACK(termit_on_new_tab), NULL);
    GtkWidget *mi_close_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
    g_signal_connect(G_OBJECT(mi_close_tab), "activate", G_CALLBACK(termit_on_close_tab), NULL);
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    GtkWidget *mi_exit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(G_OBJECT(mi_exit), "activate", G_CALLBACK(termit_on_exit), NULL);

    GtkWidget *mi_file = gtk_menu_item_new_with_label(_("File"));
    GtkWidget *file_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_new_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_close_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), separator1);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), mi_exit);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_file), file_menu);

    gtk_menu_bar_append(menu_bar, mi_file);

    // Edit menu
    GtkWidget *mi_set_tab_name = gtk_menu_item_new_with_label(_("Set tab name..."));
    g_signal_connect(G_OBJECT(mi_set_tab_name), "activate", G_CALLBACK(termit_on_set_tab_name), NULL);
    GtkWidget *mi_select_font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
    g_signal_connect(G_OBJECT(mi_select_font), "activate", G_CALLBACK(termit_on_select_font), NULL);
    GtkWidget *mi_select_foreground_color = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_COLOR, NULL);
    g_signal_connect(G_OBJECT(mi_select_foreground_color), "activate", G_CALLBACK(termit_on_select_tab_foreground_color), NULL);
    GtkWidget *separator2 = gtk_separator_menu_item_new();
    GtkWidget *mi_copy = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
    g_signal_connect(G_OBJECT(mi_copy), "activate", G_CALLBACK(termit_on_copy), NULL);
    GtkWidget *mi_paste = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
    g_signal_connect(G_OBJECT(mi_paste), "activate", G_CALLBACK(termit_on_paste), NULL);


    GtkWidget *mi_edit = gtk_menu_item_new_with_label(_("Edit"));
    GtkWidget *edit_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), separator2);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_set_tab_name);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_select_font);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_select_foreground_color);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_edit), edit_menu);

    gtk_menu_bar_append(menu_bar, mi_edit);

    // Sessions menu
    GtkWidget *mi_load_session = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    g_signal_connect(G_OBJECT(mi_load_session), "activate", G_CALLBACK(termit_on_load_session), NULL);
    GtkWidget *mi_save_session = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, NULL);
    g_signal_connect(G_OBJECT(mi_save_session), "activate", G_CALLBACK(termit_on_save_session), NULL);

    GtkWidget *mi_sessions = gtk_menu_item_new_with_label(_("Sessions"));
    GtkWidget *sessions_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(sessions_menu), mi_load_session);
    gtk_menu_shell_append(GTK_MENU_SHELL(sessions_menu), mi_save_session);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_sessions), sessions_menu);

    gtk_menu_bar_append(menu_bar, mi_sessions);

    // Encoding menu
    TRACE("%s: configs.encodings->len=%d", __FUNCTION__, configs.encodings->len);
    if (configs.encodings->len) {
        GtkWidget *mi_encodings = gtk_menu_item_new_with_label(_("Encoding"));
        GtkWidget *enc_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_encodings), enc_menu);

        gint i=0;
        for (; i<configs.encodings->len; ++i) {
            TRACE("%s", g_array_index(configs.encodings, gchar*, i));
            GtkWidget* mi_enc = gtk_menu_item_new_with_label(g_array_index(configs.encodings, gchar*, i));
            gtk_menu_shell_append(GTK_MENU_SHELL(enc_menu), mi_enc);
            
            g_signal_connect(G_OBJECT(mi_enc), "activate", 
                G_CALLBACK(termit_on_set_encoding), g_array_index(configs.encodings, gchar*, i));
        }
        gtk_menu_bar_append(menu_bar, mi_encodings);
    }

    // User menus
    TRACE("user_menus->len=%d", configs.user_menus->len);
    gint j = 0;
    for (; j<configs.user_menus->len; ++j) {
        struct UserMenu* um = &g_array_index(configs.user_menus, struct UserMenu, j);

        GtkWidget *mi_util = gtk_menu_item_new_with_label(um->name);
        GtkWidget *utils_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_util), utils_menu);
        
        TRACE("%s items->len=%d", um->name, um->items->len);
        
        gint i=0;
        for (; i<um->items->len; ++i) {
            GtkWidget *mi_tmp = gtk_menu_item_new_with_label(
                g_array_index(um->items, struct UserMenuItem, i).name);
            g_signal_connect(G_OBJECT(mi_tmp), "activate", G_CALLBACK(termit_on_user_menu_item_selected),
                &g_array_index(um->items, struct UserMenuItem, i));
            gtk_menu_shell_append(GTK_MENU_SHELL(utils_menu), mi_tmp);
        }
        gtk_menu_bar_append(menu_bar, mi_util);
    }
    termit.menu_bar = menu_bar;
}

void termit_create_popup_menu()
{
    termit.menu = gtk_menu_new();
    
    GtkWidget *mi_new_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
    GtkWidget *mi_close_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    GtkWidget *mi_set_tab_name = gtk_menu_item_new_with_label(_("Set tab name..."));
    GtkWidget *mi_select_font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
    GtkWidget *mi_select_foreground_color = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_COLOR, NULL);
    GtkWidget *separator2 = gtk_separator_menu_item_new();
    GtkWidget *mi_copy = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
    GtkWidget *mi_paste = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
    GtkWidget *separator3 = gtk_separator_menu_item_new();
    GtkWidget *mi_exit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    termit.mi_show_scrollbar = gtk_check_menu_item_new_with_label(_("Scrollbar"));

    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_new_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_close_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator1);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_set_tab_name);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_select_font);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_select_foreground_color);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), termit.mi_show_scrollbar);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator2);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator3);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_exit);

    g_signal_connect(G_OBJECT(mi_new_tab), "activate", G_CALLBACK(termit_on_new_tab), NULL);
    g_signal_connect(G_OBJECT(mi_set_tab_name), "activate", G_CALLBACK(termit_on_set_tab_name), NULL);
    g_signal_connect(G_OBJECT(mi_select_font), "activate", G_CALLBACK(termit_on_select_font), NULL);
    g_signal_connect(G_OBJECT(mi_select_foreground_color), "activate", G_CALLBACK(termit_on_select_tab_foreground_color), NULL);
    g_signal_connect(G_OBJECT(termit.mi_show_scrollbar), "toggled", G_CALLBACK(termit_on_toggle_scrollbar), NULL);
    g_signal_connect(G_OBJECT(mi_close_tab), "activate", G_CALLBACK(termit_on_close_tab), NULL);
    g_signal_connect(G_OBJECT(mi_copy), "activate", G_CALLBACK(termit_on_copy), NULL);
    g_signal_connect(G_OBJECT(mi_paste), "activate", G_CALLBACK(termit_on_paste), NULL);    
    g_signal_connect(G_OBJECT(mi_exit), "activate", G_CALLBACK(termit_on_exit), NULL);    

    ((GtkCheckMenuItem*)termit.mi_show_scrollbar)->active = configs.show_scrollbar;

    TRACE("%s: configs.encodings->len=%d", __FUNCTION__, configs.encodings->len);
    if (configs.encodings->len) {
        GtkWidget *mi_encodings = gtk_menu_item_new_with_label(_("Encoding"));
        GtkWidget *enc_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_encodings), enc_menu);
        gint i = 0;
        for (; i<configs.encodings->len; ++i) {
            GtkWidget* mi_enc = gtk_menu_item_new_with_label(g_array_index(configs.encodings, gchar*, i));
            gtk_menu_shell_append(GTK_MENU_SHELL(enc_menu), mi_enc);
            g_signal_connect(G_OBJECT(mi_enc), "activate", 
                G_CALLBACK(termit_on_set_encoding), g_array_index(configs.encodings, gchar*, i));
        }

        gtk_menu_shell_insert(GTK_MENU_SHELL(termit.menu), mi_encodings, 5);
    }
    
    // User popup menus
    TRACE("user_popup_menus->len=%d", configs.user_popup_menus->len);
    gint j = 0;
    for (; j<configs.user_popup_menus->len; ++j) {
        struct UserMenu* um = &g_array_index(configs.user_popup_menus, struct UserMenu, j);

        GtkWidget *mi_util = gtk_menu_item_new_with_label(um->name);
        GtkWidget *utils_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_util), utils_menu);
        
        TRACE("%s items->len=%d", um->name, um->items->len);
        
        gint i = 0;
        for (; i<um->items->len; i++) {
            GtkWidget *mi_tmp = gtk_menu_item_new_with_label(
                g_array_index(um->items, struct UserMenuItem, i).name);
            gtk_menu_shell_append(GTK_MENU_SHELL(utils_menu), mi_tmp);
            g_signal_connect(G_OBJECT(mi_tmp), "activate", 
                G_CALLBACK(termit_on_user_menu_item_selected), &g_array_index(um->items, struct UserMenuItem, i));
        }
        gtk_menu_shell_insert(GTK_MENU_SHELL(termit.menu), mi_util, 6);
    }
    gtk_widget_show_all(termit.menu);
}

static void termit_init(const gchar* initFile, const gchar* command)
{
    termit_init_sessions();
    termit_set_default_options();

    termit.tab_max_number = 1;
    create_main_widgets(command);

    termit_lua_init(initFile);
    
    termit_create_menubar();
    pack_widgets();
    termit_create_popup_menu();

    termit_set_font(configs.default_font);
}

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
"    --help             - print this help message\n"
"    --version          - print version number\n"
"    --execute          - execute command\n"
"    --init=init_file   - use init_file instead of standart init\n", PACKAGE_VERSION);
}

int main(int argc, char **argv)
{
    gchar* initFile = NULL;
    gchar* command = NULL;
    while (1) {
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'v'},
            {"execute", required_argument, 0, 'e'},
            {"init", required_argument, 0, 'i'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int flag = getopt_long(argc, argv, "hvi:e:", long_options, &option_index);

        /* Detect the end of the options. */
        if (flag == -1)
            break;

        switch (flag) {
        case 'h':
            termit_print_usage();
            return 0;
        case 'v':
            g_printf(PACKAGE_VERSION);
            g_printf("\n");
            return 0;
        case 'e':
            command = g_strdup(optarg);
            break;
        case 'i':
            initFile = g_strdup(optarg);
            break;
        case '?':
        /* getopt_long already printed an error message. */
            break;
        default:
            return 1;
        }
    }

    bind_textdomain_codeset(PACKAGE, "UTF-8");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    gtk_set_locale();
    
    gtk_init(&argc, &argv);

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

    /* Show the application window */
    gtk_widget_show_all(termit.main_window);
    
    // actions after display
    termit_after_show_all();
  
    gtk_main();
    termit_deinit_config();
    termit_lua_close();
    return 0;
}

