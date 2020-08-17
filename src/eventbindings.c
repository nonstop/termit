/* Copyright © 2007-2016 Nimrod Maclomair
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "configs.h"
#include "termit_core_api.h"
#include "lua_api.h"
#include "eventbindings.h"


static gint get_event_index(const gchar* event)
{
    // DEBUG
    TRACE("get_event_index running");
    guint i = 0;
    for (; i<configs.event_bindings->len; ++i){
        struct EventBinding* eb = &g_array_index(configs.event_bindings, struct EventBinding, i);
        if( !strcmp(event, eb->name)) {
            return i;
        }
    }
    return -1;
}

void termit_event_bind(const gchar* event, int lua_callback)
{
    gint ev_index = get_event_index(event);
    if(ev_index == -1){
        struct EventBinding eb = {};
        eb.name = g_strdup(event);
        eb.lua_callback = lua_callback;
        g_array_append_val(configs.event_bindings, eb);
    } else {
        struct EventBinding* eb = &g_array_index(configs.event_bindings, struct EventBinding, ev_index);
        termit_lua_unref(&eb->lua_callback);
        eb->lua_callback = lua_callback;
    }
}

void termit_event_unbind(const gchar* event)
{
    gint ev_index = get_event_index(event);
    if(ev_index == -1){
        TRACE("VTE event [%s] not found - skipping", event);
        return;
    }
    struct EventBinding* eb = &g_array_index(configs.event_bindings, struct EventBinding, ev_index);
    termit_lua_unref(&eb->lua_callback);
    g_array_remove_index(configs.event_bindings, ev_index);
}

gboolean termit_event(const gchar* event)
{
    gint ev_index = get_event_index(event);
    if(ev_index == -1){
        TRACE("VTE event [%s] not found - skipping", event);
        return FALSE;
    }
    TRACE("VTE event [%s] running", event);
    struct EventBinding* eb = &g_array_index(configs.event_bindings, struct EventBinding, ev_index);
    termit_lua_dofunction(eb->lua_callback);
    return FALSE;   // TODO: is that correct?
}
