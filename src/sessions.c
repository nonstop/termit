#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"
#include "sessions.h"

extern lua_State* L;

static gchar* termit_get_pid_dir(pid_t pid)
{
    gchar* file = g_strdup_printf("/proc/%d/cwd", pid);
    gchar* link = g_file_read_link(file, NULL);
    g_free(file);
    return link;
}

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
        fullPath = g_strdup_printf("%s/.local/share/termit", g_getenv("HOME"));
    TRACE("%s %s", __FUNCTION__, fullPath);
    if (!parse_mkdir_path(fullPath, 0700))
        g_message(_("Unable to create directory '%s': %s"), fullPath, g_strerror(errno));
    g_free(fullPath);
}

void termit_load_session(const gchar* sessionFile)
{
    TRACE("loading sesions from %s", sessionFile);
    int s = luaL_dofile(L, sessionFile);
    termit_report_lua_error(__FILE__, __LINE__, s);
}

/**
 * saves session as lua-script
 * */
void termit_save_session(const gchar* sessionFile)
{
    TRACE("saving session to file %s", sessionFile);
    FILE* fd = g_fopen(sessionFile, "w");
    if ((intptr_t)fd == -1) {
        TRACE("failed savimg to %s", sessionFile);
        return;
    }

    guint pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(termit.notebook));

    guint i = 0;
    for (; i < pages; ++i) {
        TERMIT_GET_TAB_BY_INDEX(pTab, i);
        gchar* working_dir = termit_get_pid_dir(pTab->pid);
        gchar* groupName = g_strdup_printf("tab%d", i);
        g_fprintf(fd, "%s = {}\n", groupName);
        g_fprintf(fd, "%s.name = \"%s\"\n", groupName, gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
        g_fprintf(fd, "%s.working_dir = \"%s\"\n", groupName, working_dir);
        g_fprintf(fd, "%s.command = \"%s\"\n", groupName, pTab->command);
        g_fprintf(fd, "%s.encoding = \"%s\"\n", groupName, pTab->encoding);
        g_fprintf(fd, "openTab(%s)\n\n", groupName);
        g_free(groupName);
        g_free(working_dir);
    }
//    g_fprintf(fd, data);
    fclose(fd);
}

