#include <stdlib.h>
#include <X11/Xlib.h>

#include "utils.h"
#include "callbacks.h"
#include "configs.h"
#include "keybindings.h"

extern struct Configs configs;

static gchar *default_encoding = "UTF-8";
static gchar *default_tab_name = "Terminal";
static gchar *default_font = "Monospace 10";
static gint scrollback_lines = 4096;
static gchar *default_word_chars = "-A-Za-z0-9,./?%&#:_~";
static gdouble default_transparent_saturation = 0.2;
static guint default_cols = 80;
static guint default_rows = 24;

/**
 * print error message, free GError
 * */
static void config_error(GError** err)
{
    TRACE("%s %s", __FUNCTION__, (*err)->message);
    g_error_free(*err);
    *err = NULL;
}
static void set_default_value(gchar** var, gchar* value, GError** err)
{
    *var = value;
    config_error(err);
}

static void load_termit_options(GKeyFile *keyfile)
{
    gchar *value = NULL;
    GError * error = NULL;
    value = g_key_file_get_value(keyfile, "termit", "default_tab_name", &error);
    if (!value)
        set_default_value(&configs.default_tab_name, default_tab_name, &error);
    else
        configs.default_tab_name = value;
    TRACE("default_tab_name=%s", configs.default_tab_name);

    value = g_key_file_get_value(keyfile, "termit", "default_encoding", &error);
    if (!value)
        set_default_value(&configs.default_encoding, default_encoding, &error);
    else
        configs.default_encoding = value;
    TRACE("default_encoding=%s", configs.default_encoding);
    
    value = g_key_file_get_value(keyfile, "termit", "word_chars", &error);
    if (!value)
        set_default_value(&configs.default_word_chars, default_word_chars, &error);
    else
        configs.default_word_chars = value;
    TRACE("default_word_chars=%s", configs.default_word_chars);

    value = g_key_file_get_value(keyfile, "termit", "default_font", &error);
    if (!value)
        set_default_value(&configs.default_font, default_font, &error);
    else
        configs.default_font = value;
    TRACE("default_font=%s", configs.default_font);
    

    configs.transparent_background = g_key_file_get_boolean(keyfile, "termit", "transparent_background", &error);
    if (error)
    {
        configs.transparent_background = FALSE;
        config_error(&error);
    }
    TRACE("transparent_background=%d", configs.transparent_background);
    
    if  (configs.transparent_background)
    {
        value = g_key_file_get_value(keyfile, "termit", "transparent_saturation", &error);
        if (!value)
        {
            configs.transparent_saturation = default_transparent_saturation;
            config_error(&error);
        }
        else
        {
            configs.transparent_saturation = atof(value);
            if (configs.transparent_saturation > 1
                || configs.transparent_saturation < 0)
                configs.transparent_saturation = default_transparent_saturation;
        }
        TRACE("transparent_saturation=%f", configs.transparent_saturation);
    }

    configs.hide_single_tab = g_key_file_get_boolean(keyfile, "termit", "hide_single_tab", &error);
    if (error)
    {
        configs.hide_single_tab = FALSE;
        config_error(&error);
    }

    configs.scrollback_lines = g_key_file_get_integer(keyfile, "termit", "scrollback_lines", &error);
    if (error)
    {
        configs.scrollback_lines = scrollback_lines;
        config_error(&error);
    }
        
    if (!configs.scrollback_lines)
        configs.scrollback_lines = scrollback_lines;
    TRACE("scrollback_lines=%d", configs.scrollback_lines);

    if (g_key_file_has_key(keyfile, "termit", "encodings", &error) == TRUE)
    {
        gsize enc_length = 0;
        gchar **encodings = g_key_file_get_string_list(keyfile, "termit", "encodings", &enc_length, &error);
        if (!encodings)
        {
            config_error(&error);
            return;
        }
        configs.encodings = encodings;
        configs.enc_length = enc_length;
    }
    TRACE("configs.enc_length=%d", configs.enc_length);

    value = g_key_file_get_value(keyfile, "termit", "geometry", &error);
    if (!value)
    {
        configs.cols = default_cols;
        configs.rows = default_rows;
        config_error(&error);
    }
    else
    {
        int tmp1, tmp2;
        XParseGeometry(value, &tmp1, &tmp2, 
            &configs.cols, &configs.rows);
        if ((configs.cols == 0) 
            || (configs.rows == 0))
        {
            configs.cols = default_cols;
            configs.rows = default_rows;
        }
    }
    TRACE("geometry: cols=%d, rows=%d", configs.cols, configs.rows);
}

static void set_termit_options()
{
    configs.default_tab_name = default_tab_name;
    configs.scrollback_lines = scrollback_lines;
    configs.default_font = default_font;
}

static void load_bookmark_options(GKeyFile *keyfile)
{
    GError * error = NULL;
    gsize len = 0;
    gchar **names = g_key_file_get_keys(keyfile, "bookmarks", &len, &error);
    
    int i=0;
    for (i=0; i<len; i++)
    {
        struct Bookmark b;
        b.name = g_strdup(names[i]);
        b.path = g_key_file_get_value(keyfile, "bookmarks", b.name, &error);
        TRACE("%s %s", b.name, b.path);
        g_array_append_val(configs.bookmarks, b);
    }

    g_strfreev(names);
}

void termit_load_config()
{
    configs.bookmarks = g_array_new(FALSE, TRUE, sizeof(struct Bookmark));
    configs.key_bindings = g_array_new(FALSE, TRUE, sizeof(struct KeyBindging));

    GKeyFile * keyfile = g_key_file_new();
    GError * error = NULL;
    
    const gchar *configFile = "termit.cfg";
    const gchar *configHome = g_getenv("XDG_CONFIG_HOME");
    gchar* fullPath = NULL;
    if (configHome)
        fullPath = g_strdup_printf("%s/termit/%s", configHome, configFile);
    else
    {
        fullPath = g_strdup_printf("%s/.config/termit/%s", g_getenv("HOME"), configFile);
    }
    gboolean configsFound = g_key_file_load_from_file(keyfile, fullPath, G_KEY_FILE_NONE, &error);
    g_free(fullPath);

    if (configsFound)
    {
        TRACE_MSG("config file found");
        if (g_key_file_has_group(keyfile, "termit") == TRUE)
            load_termit_options(keyfile);
        else
            set_termit_options();

        if (g_key_file_has_group(keyfile, "bookmarks") == TRUE)
            load_bookmark_options(keyfile);
        if (g_key_file_has_group(keyfile, "keybindings") == TRUE)
            termit_load_keybindings(keyfile);
        else
            termit_load_default_keybindings();
        g_key_file_free(keyfile);
    }
    else
    {
        TRACE_MSG("config file not found");
        set_termit_options();
    }
}

