/* Copyright © 2007-2016 Evgeny Ratnikov
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

#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "configs.h"
#include "termit_core_api.h"
#include "lua_api.h"
#include "keybindings.h"

extern lua_State* L;

static Display* disp;

void termit_keys_trace()
{
#ifdef DEBUG
    guint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        TRACE("%s: %d, %d(%ld)", kb->name, kb->kws.state, kb->kws.keyval, kb->keycode);
    }
#endif
}
#define ADD_DEFAULT_KEYBINDING(keybinding_, lua_callback_) \
{ \
lua_getglobal(ls, lua_callback_); \
int func = luaL_ref(ls, LUA_REGISTRYINDEX); \
termit_keys_bind(keybinding_, func); \
}

void termit_keys_set_defaults()
{
    lua_State* ls = L;
    disp = XOpenDisplay(NULL);
    ADD_DEFAULT_KEYBINDING("Alt-Left", "prevTab");
    ADD_DEFAULT_KEYBINDING("Alt-Right", "nextTab");
    ADD_DEFAULT_KEYBINDING("Ctrl-t", "openTab");
    ADD_DEFAULT_KEYBINDING("CtrlShift-w", "closeTab");
    ADD_DEFAULT_KEYBINDING("Ctrl-Insert", "copy");
    ADD_DEFAULT_KEYBINDING("Shift-Insert", "paste");
    // push func to stack, get ref
    termit_keys_trace();
}

struct TermitModifier {
    const gchar* name;
    guint state;
};
struct TermitModifier termit_modifiers[] =
{
    {"Alt", GDK_MOD1_MASK}, 
    {"Ctrl", GDK_CONTROL_MASK},
    {"Shift", GDK_SHIFT_MASK},
    {"Meta", GDK_META_MASK},
    {"Super", GDK_SUPER_MASK},
    {"Hyper", GDK_HYPER_MASK}
};
static guint TermitModsSz = sizeof(termit_modifiers)/sizeof(struct TermitModifier);

static gint get_modifier_state(const gchar* token)
{
    size_t modifier_len;
    size_t step;

    if (!token) {
        return GDK_NOTHING;
    }
    guint i = 0;
    gint state = 0;
    while (strlen(token) > 0) {
        step = 0;
        for (; i<TermitModsSz; ++i) {
            modifier_len = strlen(termit_modifiers[i].name);
            if (!strncmp(token, termit_modifiers[i].name, modifier_len)) {
                state |= termit_modifiers[i].state;
                step = modifier_len;
                break;
            }
        }
        if (step == 0) {
            return GDK_NOTHING;
        }
        token += step;
    }
    return (state == 0) ? GDK_NOTHING : state;
}

static gint get_kb_index(const gchar* name)
{
    guint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        if (!strcmp(kb->name, name)) {
            return i;
        }
    }
    return -1;
}

struct TermitMouseEvent {
    const gchar* name;
    GdkEventType type;
};
struct TermitMouseEvent termit_mouse_events[] =
{
    {"DoubleClick", GDK_2BUTTON_PRESS}
};
static guint TermitMouseEventsSz = sizeof(termit_mouse_events)/sizeof(struct TermitMouseEvent);

gint get_mouse_event_type(const gchar* event_name)
{
    if (!event_name) {
        return GDK_NOTHING;
    }
    guint i = 0;
    for (; i<TermitMouseEventsSz; ++i) {
        if (!strcmp(event_name, termit_mouse_events[i].name)) {
            return termit_mouse_events[i].type;
        }
    }
    return GDK_NOTHING;
}

static gint get_mb_index(GdkEventType type)
{
    guint i = 0;
    for (; i<configs.mouse_bindings->len; ++i) {
        struct MouseBinding* mb = &g_array_index(configs.mouse_bindings, struct MouseBinding, i);
        if (type == mb->type) {
            return i;
        }
    }
    return -1;
}

void termit_keys_unbind(const gchar* keybinding)
{
    gint kb_index = get_kb_index(keybinding);
    if (kb_index < 0) {
        TRACE("keybinding [%s] not found - skipping", keybinding);
        return;
    }
    struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, kb_index);
    termit_lua_unref(&kb->lua_callback);
    g_free(kb->name);
    g_array_remove_index(configs.key_bindings, kb_index);
}

int termit_parse_keys_str(const gchar* keybinding, struct KeyWithState* kws)
{
    gchar *modifier = NULL, *key = NULL;
    // token[0] - modifier. Only Alt, Ctrl or Shift allowed.
    gchar** tokens = g_strsplit(keybinding, "-", 2);
    if (!tokens[0]) {
        ERROR("failed to parse: [%s]", keybinding);
        return -1;
    }
    if (!tokens[1]) {
        key = tokens[0];
    } else {
        modifier = tokens[0];
        key = tokens[1];
    }
    gint tmp_state = 0;
    if (modifier) {
        tmp_state = get_modifier_state(modifier);
        if (tmp_state == GDK_NOTHING) {
            TRACE("Bad modifier: %s", keybinding);
            return -1;
        }
    }
    guint tmp_keyval = gdk_keyval_from_name(key);
    if (tmp_keyval == GDK_KEY_VoidSymbol) {
        TRACE("Bad keyval: %s", keybinding);
        return -1;
    }
    g_strfreev(tokens);
    kws->state = tmp_state;
    kws->keyval = gdk_keyval_to_lower(tmp_keyval);
    return 0;
}

void termit_keys_bind(const gchar* keybinding, int lua_callback)
{
    struct KeyWithState kws = {};
    if (termit_parse_keys_str(keybinding, &kws) < 0) {
        ERROR("failed to parse keybinding: %s", keybinding);
        return;
    }
    
    gint kb_index = get_kb_index(keybinding);
    if (kb_index < 0) {
        struct KeyBinding kb = {};
        kb.name = g_strdup(keybinding);
        kb.kws = kws;
        kb.keycode = XKeysymToKeycode(disp, kb.kws.keyval);
        kb.lua_callback = lua_callback;
        g_array_append_val(configs.key_bindings, kb);
    } else {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, kb_index);
        kb->kws = kws;
        kb->keycode = XKeysymToKeycode(disp, kb->kws.keyval);
        termit_lua_unref(&kb->lua_callback);
        kb->lua_callback = lua_callback;
    }
}

void termit_mouse_bind(const gchar* mouse_event, int lua_callback)
{
    GdkEventType type = get_mouse_event_type(mouse_event);
    if (type == GDK_NOTHING) {
        TRACE("unknown event: %s", mouse_event);
        return;
    }
    gint mb_index = get_mb_index(type);
    if (mb_index < 0) {
        struct MouseBinding mb = {};
        mb.type = type;
        mb.lua_callback = lua_callback;
        g_array_append_val(configs.mouse_bindings, mb);
    } else {
        struct MouseBinding* mb = &g_array_index(configs.mouse_bindings, struct MouseBinding, mb_index);
        mb->type = type;
        termit_lua_unref(&mb->lua_callback);
        mb->lua_callback = lua_callback;
    }
}

void termit_mouse_unbind(const gchar* mouse_event)
{
    GdkEventType type = get_mouse_event_type(mouse_event);
    if (type == GDK_NOTHING) {
        TRACE("unknown event: %s", mouse_event);
        return;
    }
    gint mb_index = get_mb_index(type);
    if (mb_index < 0) {
        TRACE("mouse event [%d] not found - skipping", type);
        return;
    }
    struct MouseBinding* mb = &g_array_index(configs.mouse_bindings, struct MouseBinding, mb_index);
    termit_lua_unref(&mb->lua_callback);
    g_array_remove_index(configs.mouse_bindings, mb_index);
}

static gboolean termit_key_press_use_keycode(GdkEventKey *event)
{
    guint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        if (kb && (event->state & kb->kws.state) == kb->kws.state) {
            if (event->hardware_keycode == kb->keycode) {
                termit_lua_dofunction(kb->lua_callback);
                return TRUE;
            }
        }
    }
    return FALSE;
}

static gboolean termit_key_press_use_keysym(GdkEventKey *event)
{
    guint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        if (kb && (event->state & kb->kws.state) == kb->kws.state) {
            if (gdk_keyval_to_lower(event->keyval) == kb->kws.keyval) {
                termit_lua_dofunction(kb->lua_callback);
                return TRUE;
            }
        }
    }
    return FALSE;
}

gboolean termit_key_event(GdkEventKey* event)
{
    switch(configs.kb_policy) {
    case TermitKbUseKeycode:
        return termit_key_press_use_keycode(event);
        break;
    case TermitKbUseKeysym:
        return termit_key_press_use_keysym(event);
        break;
    default:
        ERROR("unknown kb_policy: %d", configs.kb_policy);
    }
    return FALSE;
}

gboolean termit_mouse_event(GdkEventButton* event)
{
    guint i = 0;
    for (; i<configs.mouse_bindings->len; ++i) {
        struct MouseBinding* kb = &g_array_index(configs.mouse_bindings, struct MouseBinding, i);
        if (kb && (event->type & kb->type)) {
            termit_lua_dofunction(kb->lua_callback);
        }
    }
    return FALSE;
}
