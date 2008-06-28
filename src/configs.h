#ifndef CONFIGS_H
#define CONFIGS_H

#include <gtk/gtk.h>

enum TermitKbPolicy {TermitKbUseKeycode = 1, TermitKbUseKeysym = 2};

struct Configs
{
    gchar *default_tab_name;
    gchar *default_font;
    guint scrollback_lines;
    gchar *default_encoding;
    gchar *default_word_chars;
    gint transparent_background;
    gdouble transparent_saturation;
    gchar **encodings;
    guint enc_length;
    guint cols;
    guint rows;
    GArray *bookmarks;
    GArray *key_bindings;
    gboolean hide_single_tab;
    gboolean show_scrollbar;
    enum TermitKbPolicy kb_policy;    
};

struct Bookmark
{
    gchar *name;
    gchar *path;
};

void termit_set_defaults();
void termit_load_config();



#endif /* CONFIGS_H */

