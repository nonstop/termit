#ifndef CONFIGS_H
#define CONFIGS_H

struct Configs
{
    gchar *default_tab_name;
    gchar *default_font;
    guint scrollback_lines;
    gchar *default_encoding;
    gchar *default_word_chars;
    gchar **encodings;
    gint enc_length;
    GArray *bookmarks;
};
struct Bookmark
{
    gchar *name;
    gchar *path;
};


void termit_load_config();



#endif /* CONFIGS_H */

