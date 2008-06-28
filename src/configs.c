#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "utils.h"
#include "callbacks.h"
#include "configs.h"
#include "keybindings.h"

extern struct Configs configs;

/**
 * print error message, free GError
 * */
static void config_error(GError** err)
{
    TRACE("%s %s", __FUNCTION__, (*err)->message);
    g_error_free(*err);
    *err = NULL;
}

static void load_termit_options(GKeyFile *keyfile)
{
    gchar *value = NULL;
    GError * error = NULL;
    value = g_key_file_get_value(keyfile, "termit", "default_tab_name", &error);
    if (value)
        configs.default_tab_name = value;
    TRACE("default_tab_name=%s", configs.default_tab_name);

    value = g_key_file_get_value(keyfile, "termit", "default_encoding", &error);
    if (!value)
        config_error(&error);
    else
        configs.default_encoding = value;
    TRACE("default_encoding=%s", configs.default_encoding);
    
    value = g_key_file_get_value(keyfile, "termit", "word_chars", &error);
    if (!value)
        config_error(&error);
    else
        configs.default_word_chars = value;
    TRACE("default_word_chars=%s", configs.default_word_chars);

    value = g_key_file_get_value(keyfile, "termit", "default_font", &error);
    if (!value)
        config_error(&error);
    else
        configs.default_font = value;
    TRACE("default_font=%s", configs.default_font);
    
    configs.show_scrollbar = g_key_file_get_boolean(keyfile, "termit", "show_scrollbar", &error);
    if (error)
    {
        configs.show_scrollbar = True;
        config_error(&error);
    }
    TRACE("show_scrollbar=%d", configs.show_scrollbar);

    configs.transparent_background = g_key_file_get_boolean(keyfile, "termit", "transparent_background", &error);
    if (error)
    {
        configs.transparent_background = False;
        config_error(&error);
    }
    TRACE("transparent_background=%d", configs.transparent_background);
    
    if  (configs.transparent_background)
    {
        value = g_key_file_get_value(keyfile, "termit", "transparent_saturation", &error);
        if (!value)
        {
            config_error(&error);
        }
        else
        {
            gdouble tr_sat = atof(value);
            if ((tr_sat <= 1) && (tr_sat >= 0))
                configs.transparent_saturation = tr_sat;
        }
        TRACE("transparent_saturation=%f", configs.transparent_saturation);
    }

    configs.hide_single_tab = g_key_file_get_boolean(keyfile, "termit", "hide_single_tab", &error);
    if (error)
    {
        configs.hide_single_tab = False;
        config_error(&error);
    }

    guint scrollback_lines = g_key_file_get_integer(keyfile, "termit", "scrollback_lines", &error);
    if (error)
    {
        config_error(&error);
    }
        
    if (scrollback_lines)
        configs.scrollback_lines = scrollback_lines;
    TRACE("scrollback_lines=%d", configs.scrollback_lines);

    if (g_key_file_has_key(keyfile, "termit", "encodings", &error) == TRUE)
    {
        gsize enc_length = 0;
        gchar **encodings = g_key_file_get_string_list(keyfile, "termit", "encodings", &enc_length, &error);
        if (!encodings)
        {
            config_error(&error);
        }
        else
        {
            configs.encodings = encodings;
            configs.enc_length = enc_length;
        }
    }
    TRACE("configs.enc_length=%d", configs.enc_length);

    value = g_key_file_get_value(keyfile, "termit", "geometry", &error);
    if (!value)
    {
        config_error(&error);
    }
    else
    {
        uint cols, rows;
        int tmp1, tmp2;
        XParseGeometry(value, &tmp1, &tmp2, 
            &cols, &rows);
        if ((cols > 0) && (rows > 0))
        {
            configs.cols = cols;
            configs.rows = rows;
        }
    }
    TRACE("geometry: cols=%d, rows=%d", configs.cols, configs.rows);
    g_free(value);

    value = g_key_file_get_value(keyfile, "termit", "kb_policy", &error);
    if (!value)
    {
        config_error(&error);
    }
    else
    {
        if (!strcmp(value, "keycode"))
            configs.kb_policy = TermitKbUseKeycode;
        else if (!strcmp(value, "keysym"))
            configs.kb_policy = TermitKbUseKeysym;
        else
            ERROR("unknown value for kb_policy");
    }
    g_free(value);
}


static void termit_set_default_options()
{
    configs.default_tab_name = "Terminal";
    configs.default_font = "Monospace 10";
    configs.scrollback_lines = 4096;
    configs.default_encoding = "UTF-8";
    configs.default_word_chars = "-A-Za-z0-9,./?%&#_~";
    configs.transparent_background = False;
    configs.transparent_saturation = 0.2;
    configs.enc_length = 0;
    configs.cols = 80;
    configs.rows = 24;
    configs.kb_policy = TermitKbUseKeysym;    

    configs.bookmarks = g_array_new(FALSE, TRUE, sizeof(struct Bookmark));
    configs.key_bindings = g_array_new(FALSE, TRUE, sizeof(struct KeyBindging));

    configs.hide_single_tab = False;
    configs.show_scrollbar = True;
}

void termit_set_defaults()
{
    termit_set_default_options();
    termit_set_default_keybindings();
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

        if (g_key_file_has_group(keyfile, "bookmarks") == TRUE)
            load_bookmark_options(keyfile);
        if (g_key_file_has_group(keyfile, "keybindings") == TRUE)
            termit_load_keybindings(keyfile);
        g_key_file_free(keyfile);
    }
    else
    {
        TRACE_MSG("config file not found");
    }
}

