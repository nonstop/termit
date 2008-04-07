#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <glib.h>

#include "configs.h"
#include "utils.h"
#include "sessions.h"
#include "callbacks.h"

extern struct TermitData termit;
extern struct Configs configs;

// from Openbox
static gboolean parse_mkdir(const gchar *path, gint mode)
{
    gboolean ret = TRUE;

    g_return_val_if_fail(path != NULL, FALSE);
    g_return_val_if_fail(path[0] != '\0', FALSE);

    if (!g_file_test(path, G_FILE_TEST_IS_DIR))
        if (mkdir(path, mode) == -1)
            ret = FALSE;

    return ret;
}

// from Openbox
static gboolean parse_mkdir_path(const gchar *path, gint mode)
{
    gboolean ret = TRUE;

    g_return_val_if_fail(path != NULL, FALSE);
    g_return_val_if_fail(path[0] == '/', FALSE);

    if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
        gchar *c, *e;

        c = g_strdup(path);
        e = c;
        while ((e = strchr(e + 1, '/'))) {
            *e = '\0';
            if (!(ret = parse_mkdir(c, mode)))
                goto parse_mkdir_path_end;
            *e = '/';
        }
        ret = parse_mkdir(c, mode);

    parse_mkdir_path_end:
        g_free(c);
    }

    return ret;
}

void termit_init_sessions()
{
    gchar *fullPath;
    const gchar *dataHome = g_getenv("XDG_DATA_HOME");
    if (dataHome)
        fullPath = g_strdup_printf("%s/termit", dataHome);
    else
    {
        fullPath = g_strdup_printf("%s/.local/share/termit", g_getenv("HOME"));
    }
    TRACE("%s %s", __FUNCTION__, fullPath);
    if (!parse_mkdir_path(fullPath, 0700))
    {
        g_message(_("Unable to create directory '%s': %s"), fullPath, g_strerror(errno));
    }
    g_free(fullPath);
}

struct TermitSession
{
    gchar* tab_name;
    gchar* shell_cmd;
    gchar* working_dir;
    gchar* encoding;
};
static GArray* session_tabs;

static void termit_load_session_tabs(GKeyFile* kf, gint tab_count)
{
    TRACE("tab_count=%d", tab_count);
    gint i = 0;
    session_tabs = g_array_new(FALSE, TRUE, sizeof(struct TermitSession));
    for (; i < tab_count; ++i)
    {
        gchar* groupName = g_strdup_printf("tab%d", i);
        if (!g_key_file_has_group(kf, groupName))
        {
            g_free(groupName);
            continue;
        }
        TRACE("%s", groupName);
        struct TermitSession ts;
        gchar *value = NULL;
        value = g_key_file_get_value(kf, groupName, "tab_name", NULL);
        if (!value)
            ts.tab_name = g_strdup_printf("%s %d", configs.default_tab_name, i);
        else
            ts.tab_name = value;
        value = g_key_file_get_value(kf, groupName, "shell_cmd", NULL);
        if (!value)
            ts.shell_cmd = g_strdup(g_getenv("SHELL"));
        else
            ts.shell_cmd = value;
        value = g_key_file_get_value(kf, groupName, "working_dir", NULL);
        if (!value)
            ts.working_dir = g_strdup(g_getenv("PWD"));
        else
            ts.working_dir = value;
        value = g_key_file_get_value(kf, groupName, "encoding", NULL);
        if (!value)
            ts.encoding = g_strdup(configs.default_encoding);
        else
            ts.encoding = value;
        g_free(groupName);
        g_array_append_val(session_tabs, ts);
    }
    for (i=0; i<session_tabs->len; ++i)
    {
        struct TermitSession ts;
        ts = g_array_index(session_tabs, struct TermitSession, i);
        TRACE("%3d  name=%s, cmd=%s, dir=%s, encoding=%s",
                i, ts.tab_name, ts.shell_cmd, ts.working_dir, ts.encoding);
    }
}

static void free_session_tabs()
{
    guint i = 0;
    for (; i<session_tabs->len; ++i)
    {
        struct TermitSession ts = g_array_index(session_tabs, struct TermitSession, i);
        g_free(ts.tab_name);
        g_free(ts.shell_cmd);
        g_free(ts.working_dir);
        g_free(ts.encoding);
    }
    g_array_free(session_tabs, TRUE);
}

void termit_load_session(const gchar* sessionFile)
{
    TRACE("loading sesions from %s", sessionFile);
    GError* err = NULL;
    GKeyFile *kf = g_key_file_new();
    if (g_key_file_load_from_file(kf, sessionFile, G_KEY_FILE_NONE, &err) != TRUE)
    {
        TRACE("failed loading sessions: %s", g_strerror(err->code));
        return;
    }
    if (g_key_file_has_group(kf, "session") != TRUE)
    {
        TRACE_MSG("default group not found in session file");
        return;
    }
    gint tab_count = g_key_file_get_integer(kf, "session", "tab_count", &err);
    if (err && 
        ((err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
        || (err->code == G_KEY_FILE_ERROR_INVALID_VALUE)))
    {
        TRACE_MSG("failed loading tab_count");
        return;
    }
    termit_load_session_tabs(kf, tab_count);
    if (!session_tabs->len)
    {
        termit_append_tab();
        goto free_session_tabs;
    }

    guint pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    while (pages)
    {
        termit_del_tab();
        pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));
    }
    guint i = 0;
    for (; i<session_tabs->len; ++i)
    {
        struct TermitSession ts = g_array_index(session_tabs, struct TermitSession, i);
        termit_append_tab_with_details(ts.tab_name, ts.shell_cmd, ts.working_dir, ts.encoding);
    }
//  finally block
free_session_tabs:
    free_session_tabs();
}

