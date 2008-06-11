#include <gtk/gtk.h>
#include <vte/vte.h>
#include <string.h>

#include <getopt.h>

#include "config.h"

#include "utils.h"
#include "callbacks.h"
#include "sessions.h"
#include "configs.h"

struct TermitData termit;
struct Configs configs;


static GtkWidget* create_statusbar()
{
    GtkWidget* statusbar = gtk_statusbar_new();
    return statusbar;
}

static GtkWidget* create_menubar()
{
    GtkWidget* menu_bar = gtk_menu_bar_new();
    
    // File menu
    GtkWidget *mi_new_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
    g_signal_connect(G_OBJECT(mi_new_tab), "activate", G_CALLBACK(termit_new_tab), NULL);
    GtkWidget *mi_close_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
    g_signal_connect(G_OBJECT(mi_close_tab), "activate", G_CALLBACK(termit_close_tab), NULL);
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    GtkWidget *mi_exit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    g_signal_connect(G_OBJECT(mi_exit), "activate", G_CALLBACK(termit_menu_exit), NULL);
    
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
    g_signal_connect(G_OBJECT(mi_set_tab_name), "activate", G_CALLBACK(termit_set_tab_name), NULL);
    GtkWidget *mi_select_font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
    g_signal_connect(G_OBJECT(mi_select_font), "activate", G_CALLBACK(termit_select_font), NULL);
    GtkWidget *separator2 = gtk_separator_menu_item_new();
    GtkWidget *mi_copy = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
    g_signal_connect(G_OBJECT(mi_copy), "activate", G_CALLBACK(termit_copy), NULL);
    GtkWidget *mi_paste = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
    g_signal_connect(G_OBJECT(mi_paste), "activate", G_CALLBACK(termit_paste), NULL);
    

    GtkWidget *mi_edit = gtk_menu_item_new_with_label(_("Edit"));
    GtkWidget *edit_menu = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), separator2);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_set_tab_name);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), mi_select_font);
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

    // Bookmarks menu
    TRACE("bookmarks->len=%d", configs.bookmarks->len);
    if (configs.bookmarks->len)
    {
        GtkWidget *mi_bookmarks = gtk_menu_item_new_with_label(_("Bookmarks"));
        GtkWidget *bookmarks_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_bookmarks), bookmarks_menu);
        
        int i=0;
        for (i=0; i<configs.bookmarks->len; i++)
        {
            GtkWidget *mi_tmp = gtk_menu_item_new_with_label(
                g_array_index(configs.bookmarks, struct Bookmark, i).name);
            g_signal_connect(G_OBJECT(mi_tmp), "button-press-event", G_CALLBACK(termit_bookmark_selected),
                &g_array_index(configs.bookmarks, struct Bookmark, i));
//            g_signal_connect(G_OBJECT(mi_tmp), "activate", G_CALLBACK(termit_bookmark_selected),
//                g_array_index(configs.bookmarks, struct Bookmark, i).path);
            gtk_menu_shell_append(GTK_MENU_SHELL(bookmarks_menu), mi_tmp);
        }
        
        gtk_menu_bar_append(menu_bar, mi_bookmarks);
    }

    // Encoding menu
    GtkWidget *mi_encodings = gtk_menu_item_new_with_label(_("Encoding"));
    GtkWidget *enc_menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_encodings), enc_menu);
    TRACE("configs.enc_length=%d", configs.enc_length);
    if (configs.enc_length)
    {
        GtkWidget *msi_enc[configs.enc_length];
        gint i=0;
        for (i=0; i<configs.enc_length; i++)
        {
            TRACE("%s", configs.encodings[i]);
            msi_enc[i] = gtk_menu_item_new_with_label(configs.encodings[i]);
            gtk_menu_shell_append(GTK_MENU_SHELL(enc_menu), msi_enc[i]);
        }

        for (i=0; i<configs.enc_length; i++)
            g_signal_connect(G_OBJECT(msi_enc[i]), "activate", G_CALLBACK(termit_set_encoding), configs.encodings[i]);    
    }
    gtk_menu_bar_append(menu_bar, mi_encodings);
    
    return menu_bar;
}

static void create_main_window(const gchar* sessionFile, const gchar* command)
{
    termit.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    termit.statusbar = create_statusbar();
    termit.menu_bar = create_menubar();
    termit.notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);
    
    if (sessionFile)
    {
        TRACE("using session: %s", sessionFile);
        termit_load_session(sessionFile);
    }
    if (command)
    {
        TRACE("using command: %s", command);
        termit_append_tab_with_command(command);
        termit_set_statusbar_encoding(0);
    }

    if (!gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook)))
    {
        termit_append_tab();
        termit_set_statusbar_encoding(0);
    }
   
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), termit.menu_bar, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), termit.statusbar, TRUE, 1, 0);

    gtk_box_pack_start(GTK_BOX(vbox), termit.notebook, TRUE, 1, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, 1, 0);
    gtk_container_add(GTK_CONTAINER(termit.main_window), vbox);

    g_signal_connect(G_OBJECT(termit.notebook), "switch-page", G_CALLBACK(termit_switch_page), NULL);
    g_signal_connect(G_OBJECT(termit.main_window), "button-press-event", G_CALLBACK(termit_double_click), NULL);
}

static void termit_create_popup_menu()
{
    termit.menu = gtk_menu_new();
    
    GtkWidget *mi_new_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
    GtkWidget *mi_close_tab = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    GtkWidget *mi_set_tab_name = gtk_menu_item_new_with_label(_("Set tab name..."));
    GtkWidget *mi_select_font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
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
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), termit.mi_show_scrollbar);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator2);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator3);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_exit);

    g_signal_connect(G_OBJECT(mi_new_tab), "activate", G_CALLBACK(termit_new_tab), NULL);
    g_signal_connect(G_OBJECT(mi_set_tab_name), "activate", G_CALLBACK(termit_set_tab_name), NULL);
    g_signal_connect(G_OBJECT(mi_select_font), "activate", G_CALLBACK(termit_select_font), NULL);
    g_signal_connect(G_OBJECT(termit.mi_show_scrollbar), "toggled", G_CALLBACK(termit_toggle_scrollbar), NULL);
    g_signal_connect(G_OBJECT(mi_close_tab), "activate", G_CALLBACK(termit_close_tab), NULL);
    g_signal_connect(G_OBJECT(mi_copy), "activate", G_CALLBACK(termit_copy), NULL);
    g_signal_connect(G_OBJECT(mi_paste), "activate", G_CALLBACK(termit_paste), NULL);    
    g_signal_connect(G_OBJECT(mi_exit), "activate", G_CALLBACK(termit_menu_exit), NULL);    

    ((GtkCheckMenuItem*)termit.mi_show_scrollbar)->active = configs.show_scrollbar;

    TRACE("configs.enc_length=%d", configs.enc_length);
    if (configs.enc_length)
    {
        GtkWidget *mi_encodings = gtk_menu_item_new_with_label(_("Encoding"));
        GtkWidget *enc_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_encodings), enc_menu);
        GtkWidget *msi_enc[configs.enc_length];
        gint i = 0;
        for (; i<configs.enc_length; i++)
        {
            TRACE("%s", configs.encodings[i]);
            msi_enc[i] = gtk_menu_item_new_with_label(configs.encodings[i]);
            gtk_menu_shell_append(GTK_MENU_SHELL(enc_menu), msi_enc[i]);
        }

        for (i = 0; i<configs.enc_length; i++)
            g_signal_connect(G_OBJECT(msi_enc[i]), "activate", G_CALLBACK(termit_set_encoding), configs.encodings[i]);    
        
        gtk_menu_shell_insert(GTK_MENU_SHELL(termit.menu), mi_encodings, 4);
    }

    gtk_widget_show_all(termit.menu);
}

static void termit_init(const gchar* sessionFile, const gchar* command)
{
    termit.tab_max_number = 1;

    termit_create_popup_menu();
    create_main_window(sessionFile, command);
    
    termit.font = pango_font_description_from_string(configs.default_font);
    termit_set_font();
}

static void termit_print_usage()
{
    g_print(
"termit - terminal emulator\n"
"Options:\n"
"    --help                 - print this help message\n"
"    --version              - print version number\n"
"    --execute              - execute command\n"
"    --session=session_file - start session using session_file\n");
}

int main(int argc, char **argv)
{
    gchar* sessionFile = NULL;
    gchar* command = NULL;
    while (1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, 'h'},
            {"version", no_argument, 0, 'v'},
            {"execute", required_argument, 0, 'e'},
            {"session", required_argument, 0, 's'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int flag = getopt_long(argc, argv, "hvs:e:", long_options, &option_index);

        /* Detect the end of the options. */
        if (flag == -1)
            break;

        switch (flag)
        {
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
        case 's':
            sessionFile = g_strdup(optarg);
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

    termit_set_defaults();
    termit_load_config();

    termit_init_sessions();
    termit_init(sessionFile, command);
    g_free(command);
    g_free(sessionFile);

    /**
     * dirty hack from gnome-terminal ;-)
     * F10 is used in many console apps, so we change global Gtk setting for termit
     * */
    gtk_settings_set_string_property(gtk_settings_get_default(), "gtk-menu-bar-accel",
        "<Shift><Control><Mod1><Mod2><Mod3><Mod4><Mod5>F10", "termit");

    g_signal_connect(G_OBJECT (termit.main_window), "delete_event", G_CALLBACK (termit_delete_event), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "destroy", G_CALLBACK (termit_destroy), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "key-press-event", G_CALLBACK(termit_key_press), NULL);

    /* Show the application window */
    gtk_widget_show_all(termit.main_window);
    termit_hide_scrollbars();

    gtk_main();

    return 0;
}
