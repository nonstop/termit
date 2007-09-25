#include <gtk/gtk.h>
#include <vte/vte.h>
#include <string.h>

#include "utils.h"
#include "callbacks.h"
#include "configs.h"

struct TermitData termit;
struct Configs configs;


static GtkWidget* create_statusbar()
{
    GtkWidget* statusbar = gtk_statusbar_new();
    GtkWidget* label = gtk_label_new(_("goto: "));

    GtkListStore *model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING); 
    TRACE_NUM(configs.bookmarks->len);
    int i=0;
    for (i=0; i<configs.bookmarks->len; i++)
    {
        GtkTreeIter iter;
        gtk_list_store_append(model, &iter);
        gtk_list_store_set(model, &iter, 
            0, g_array_index(configs.bookmarks, struct Bookmark, i).name, 
            1, g_array_index(configs.bookmarks, struct Bookmark, i).path, 
            -1);
    }
    termit.cb_bookmarks = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (termit.cb_bookmarks), renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (termit.cb_bookmarks), renderer, "text", 0, NULL);
    gtk_widget_show_all(termit.cb_bookmarks);
        
    g_signal_connect(G_OBJECT(termit.cb_bookmarks), "changed", G_CALLBACK(termit_cb_bookmarks_changed), NULL);


    gtk_box_pack_end(GTK_BOX(statusbar), termit.cb_bookmarks, FALSE, 0, 0);
    gtk_box_pack_end(GTK_BOX(statusbar), label, FALSE, 0, 0);
    return statusbar;
}

static void create_main_window()
{
    termit.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    termit.statusbar = create_statusbar();
    termit.notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(termit.notebook), TRUE);
    termit_append_tab();
    termit_set_statusbar_encoding(0);
    gtk_box_pack_start(GTK_BOX(vbox), termit.notebook, TRUE, 1, 0);
    gtk_box_pack_start(GTK_BOX(vbox), termit.statusbar, FALSE, 0, 0);
    gtk_container_add(GTK_CONTAINER(termit.main_window), vbox);

    g_signal_connect(G_OBJECT(termit.notebook), "switch-page", G_CALLBACK(termit_switch_page), NULL);
    g_signal_connect(G_OBJECT(termit.main_window), "button-press-event", G_CALLBACK(termit_double_click), NULL);
}

static void termit_create_popup_menu()
{
    termit.menu = gtk_menu_new();
    GtkWidget *mi_new_tab = gtk_menu_item_new_with_label(_("New tab"));
	GtkWidget *mi_close_tab = gtk_menu_item_new_with_label(_("Close tab"));
    GtkWidget *separator1 = gtk_separator_menu_item_new();
    GtkWidget *mi_set_tab_name = gtk_menu_item_new_with_label(_("Set tab name..."));
    GtkWidget *mi_select_font = gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_FONT, NULL);
    GtkWidget *separator2 = gtk_separator_menu_item_new();
	GtkWidget *mi_copy = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
	GtkWidget *mi_paste = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
    GtkWidget *separator3 = gtk_separator_menu_item_new();
    GtkWidget *mi_exit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_new_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_close_tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator1);
	gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_set_tab_name);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_select_font);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator2);
	gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_copy);
	gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), separator3);
    gtk_menu_shell_append(GTK_MENU_SHELL(termit.menu), mi_exit);

    g_signal_connect(G_OBJECT(mi_new_tab), "activate", G_CALLBACK(termit_new_tab), NULL);
	g_signal_connect(G_OBJECT(mi_set_tab_name), "activate", G_CALLBACK(termit_set_tab_name), NULL);
    g_signal_connect(G_OBJECT(mi_select_font), "activate", G_CALLBACK(termit_select_font), NULL);
	g_signal_connect(G_OBJECT(mi_close_tab), "activate", G_CALLBACK(termit_close_tab), NULL);
	g_signal_connect(G_OBJECT(mi_copy), "activate", G_CALLBACK(termit_copy), NULL);
	g_signal_connect(G_OBJECT(mi_paste), "activate", G_CALLBACK(termit_paste), NULL);	
	g_signal_connect(G_OBJECT(mi_exit), "activate", G_CALLBACK(termit_menu_exit), NULL);	

    TRACE_NUM(configs.enc_length);
    if (configs.enc_length)
    {
        GtkWidget *mi_encodings = gtk_menu_item_new_with_label(_("Encodings"));
        GtkWidget *enc_menu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_encodings), enc_menu);
        GtkWidget *msi_enc[configs.enc_length];
        gint i=0;
        for (i=0; i<configs.enc_length; i++)
        {
            TRACE_STR(configs.encodings[i]);
            msi_enc[i] = gtk_menu_item_new_with_label(configs.encodings[i]);
            gtk_menu_shell_append(GTK_MENU_SHELL(enc_menu), msi_enc[i]);
        }

        for (i=0; i<configs.enc_length; i++)
            g_signal_connect(G_OBJECT(msi_enc[i]), "activate", G_CALLBACK(termit_set_encoding), configs.encodings[i]);	
        
        gtk_menu_shell_insert(GTK_MENU_SHELL(termit.menu), mi_encodings, 4);
    }

    gtk_widget_show_all(termit.menu);
}

static void termit_init()
{
    termit.tabs = g_array_new(FALSE, TRUE, sizeof(struct TermitTab));
    termit.tab_max_number = 1;

    termit_create_popup_menu();
    create_main_window();
    
    termit.font = pango_font_description_from_string(configs.default_font);
    termit_set_font();
}

int main(int argc, char **argv)
{
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    gtk_set_locale();
    
    termit_load_config();
    gtk_init(&argc, &argv);

    termit_init();

    g_signal_connect(G_OBJECT (termit.main_window), "delete_event", G_CALLBACK (termit_delete_event), NULL);
    g_signal_connect(G_OBJECT (termit.main_window), "destroy", G_CALLBACK (termit_destroy), NULL);
	g_signal_connect(G_OBJECT (termit.main_window), "key-press-event", G_CALLBACK(termit_key_press), NULL);
      /* Set up our GUI elements */

    /* Show the application window */
    gtk_widget_show_all(termit.main_window);

    gtk_main();

    return 0;
}
