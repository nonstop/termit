/*  Copyright (C) 2007-2010, Evgeny Ratnikov

    This file is part of termit.
    termit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 
    as published by the Free Software Foundation.
    termit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with termit. If not, see <http://www.gnu.org/licenses/>.*/

#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"
#include "sessions.h"

extern lua_State* L;

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
    termit_lua_report_error(__FILE__, __LINE__, s);
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
        g_fprintf(fd, "%s.title = \"%s\"\n", groupName, gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
        g_fprintf(fd, "%s.workingDir = \"%s\"\n", groupName, working_dir);
        g_fprintf(fd, "%s.command = \"%s\"\n", groupName, pTab->argv[0]);
        // FIXME compund commands would not be saved
        g_fprintf(fd, "%s.argv = {}\n", groupName);
        g_fprintf(fd, "%s.encoding = \"%s\"\n", groupName, pTab->encoding);
        GValue val = {};
        g_value_init(&val, g_type_from_name("VteTerminalEraseBinding"));
        g_object_get_property(G_OBJECT(pTab->vte), "backspace-binding", &val);
        VteTerminalEraseBinding eb = g_value_get_enum(&val);
        if (eb != VTE_ERASE_AUTO) {
            g_fprintf(fd, "%s.backspaceBinding = \"%s\"\n", groupName, termit_erase_binding_to_string(eb));
        }
        g_value_unset(&val);
        g_value_init(&val, g_type_from_name("VteTerminalEraseBinding"));
        g_object_get_property(G_OBJECT(pTab->vte), "delete-binding", &val);
        eb = g_value_get_enum(&val);
        if (eb != VTE_ERASE_AUTO) {
            g_fprintf(fd, "%s.deleteBinding = \"%s\"\n", groupName, termit_erase_binding_to_string(eb));
        }
        g_value_unset(&val);
        g_fprintf(fd, "openTab(%s)\n\n", groupName);
        g_free(groupName);
        g_free(working_dir);
    }
//    g_fprintf(fd, data);
    fclose(fd);
}

