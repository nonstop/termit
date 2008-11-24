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

void trace_keybindings()
{
#ifdef DEBUG
    TRACE_MSG("");
    TRACE("len: %d", configs.key_bindings->len);
    gint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        TRACE("%s: %d, %d(%ld)", kb->name, kb->state, kb->keyval, kb->keycode);
    }
    TRACE_MSG("");
#endif
}
#define ADD_DEFAULT_KEYBINDING(keybinding_, lua_callback_) \
{ \
lua_getglobal(ls, lua_callback_); \
int func = luaL_ref(ls, LUA_REGISTRYINDEX); \
termit_bind_key(keybinding_, func); \
}

void termit_set_default_keybindings()
{
    lua_State* ls = L;
    disp = XOpenDisplay(NULL);
    ADD_DEFAULT_KEYBINDING("Alt-Left", "prevTab");
    ADD_DEFAULT_KEYBINDING("Alt-Right", "nextTab");
    ADD_DEFAULT_KEYBINDING("Ctrl-t", "openTab");
    ADD_DEFAULT_KEYBINDING("Ctrl-w", "closeTab");
    ADD_DEFAULT_KEYBINDING("Ctrl-Insert", "copy");
    ADD_DEFAULT_KEYBINDING("Shift-Insert", "paste");
    // push func to stack, get ref
    trace_keybindings();
}

struct TermitModifier {
    gchar* name;
    guint state;
};
struct TermitModifier termit_modifiers[] =
{
    {"Alt", GDK_MOD1_MASK}, 
    {"Ctrl", GDK_CONTROL_MASK},
    {"Shift", GDK_SHIFT_MASK}
};
static guint TermitModsSz = sizeof(termit_modifiers)/sizeof(struct TermitModifier);

static guint get_modifier_state(const gchar* token)
{
    if (!token)
        return 0;
    gint i = 0;
    for (; i<TermitModsSz; ++i) {
        if (!strcmp(token, termit_modifiers[i].name))
            return termit_modifiers[i].state;
    }
    return 0;
}

static gint get_kb_index(const gchar* name)
{
    gint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        if (!strcmp(kb->name, name))
            return i;
    }
    return -1;
}

void termit_bind_key(const gchar* keybinding, int lua_callback)
{
    gchar** tokens = g_strsplit(keybinding, "-", 2);
    // token[0] - modifier. Only Alt, Ctrl or Shift allowed.
    if (!tokens[0] || !tokens[1])
        return;
    guint tmp_state = get_modifier_state(tokens[0]);
    if (!tmp_state) {
        TRACE("Bad modifier: %s", keybinding);
        return;
    }
    // token[1] - key. Only alfabet and numeric keys allowed.
    guint tmp_keyval = gdk_keyval_from_name(tokens[1]);
    if (tmp_keyval == GDK_VoidSymbol) {
        TRACE("Bad keyval: %s", keybinding);
        return;
    }
//    TRACE("%s: %s(%d), %s(%d)", kb->name, tokens[0], tmp_state, tokens[1], tmp_keyval);
    g_strfreev(tokens);
    
    gint kb_index = get_kb_index(keybinding);
    if (kb_index < 0) {
        struct KeyBinding kb = {0};
        kb.name = g_strdup(keybinding);
        kb.state = tmp_state;
        kb.keyval = gdk_keyval_to_lower(tmp_keyval);
        kb.keycode = XKeysymToKeycode(disp, kb.keyval);
        kb.lua_callback = lua_callback;
        g_array_append_val(configs.key_bindings, kb);
    } else {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, kb_index);
        kb->state = tmp_state;
        kb->keyval = gdk_keyval_to_lower(tmp_keyval);
        kb->keycode = XKeysymToKeycode(disp, kb->keyval);
        kb->lua_callback = lua_callback;
    }
}

static gboolean termit_key_press_use_keycode(GdkEventKey *event)
{
    gint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        if (kb && (event->state & kb->state))
            if (event->hardware_keycode == kb->keycode) {
                termit_lua_dofunction(kb->lua_callback);
                return TRUE;
            }
    }
    return FALSE;
}

static gboolean termit_key_press_use_keysym(GdkEventKey *event)
{
    gint i = 0;
    for (; i<configs.key_bindings->len; ++i) {
        struct KeyBinding* kb = &g_array_index(configs.key_bindings, struct KeyBinding, i);
        if (kb && (event->state & kb->state))
            if (gdk_keyval_to_lower(event->keyval) == kb->keyval) {
                termit_lua_dofunction(kb->lua_callback);
                return TRUE;
            }
    }
    return FALSE;
}

gboolean termit_process_key(GdkEventKey* event)
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

