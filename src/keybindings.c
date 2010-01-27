#include <stdlib.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>

#include "keybindings.h"
#include "configs.h"
#include "utils.h"
#include "callbacks.h"

extern struct Configs configs;

static Display* disp;

static void trace_keybindings()
{
#ifdef DEBUG
    TRACE_MSG("");
    int i = 0;
    for (; i<configs.key_bindings->len; ++i)
    {
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, i);
        TRACE("%s: %d, %d(%ld), %s", 
            kb->name, kb->state, kb->keyval, kb->keycode, kb->default_binding);
    }
    TRACE_MSG("");
#endif
}

void termit_set_default_keybindings()
{
    TRACE_MSG(__FUNCTION__);
#define ADD_DEFAULT_KEYBINDING(name_, state_, keyval_, callback_, default_binding_) \
    kb.name = name_; \
    kb.state = state_; \
    kb.keyval = keyval_; \
    kb.callback = callback_; \
    kb.keycode = XKeysymToKeycode(disp, keyval_); \
    kb.default_binding = default_binding_; \
    g_array_append_val(configs.key_bindings, kb);

    disp = XOpenDisplay(NULL);

    struct KeyBindging kb;
    ADD_DEFAULT_KEYBINDING("prev_tab", GDK_MOD1_MASK, GDK_Left, termit_prev_tab, "Alt-Left");
    ADD_DEFAULT_KEYBINDING("next_tab", GDK_MOD1_MASK, GDK_Right, termit_next_tab, "Alt-Right");
    ADD_DEFAULT_KEYBINDING("open_tab", GDK_CONTROL_MASK, GDK_t, termit_new_tab, "Ctrl-t");
    ADD_DEFAULT_KEYBINDING("close_tab", GDK_CONTROL_MASK, GDK_w, termit_close_tab, "Ctrl-w");
    ADD_DEFAULT_KEYBINDING("copy", GDK_CONTROL_MASK, GDK_Insert, termit_copy, "Ctrl-Insert");
    ADD_DEFAULT_KEYBINDING("paste", GDK_SHIFT_MASK, GDK_Insert, termit_paste, "Shift-Insert");

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
    int i = 0;
    for (; i<TermitModsSz; ++i)
    {
        if (!strcmp(token, termit_modifiers[i].name))
            return termit_modifiers[i].state;
    }
    return 0;
}

static void set_keybinding(struct KeyBindging* kb, const gchar* value)
{
    if (!strcmp(value, "None"))
    {
        kb->state = 0;
        kb->keyval = 0;
        return;
    }
    gchar** tokens = g_strsplit(value, "-", 2);
    // token[0] - modifier. Only Alt, Ctrl or Shift allowed.
    if (!tokens[0] || !tokens[1])
        return;
    guint tmp_state = get_modifier_state(tokens[0]);
    if (!tmp_state)
        return;
    // token[1] - key. Only alfabet and numeric keys allowed.
    guint tmp_keyval = gdk_keyval_from_name(tokens[1]);
    if (tmp_keyval == GDK_VoidSymbol)
        return;
    TRACE("%s: %s(%d), %s(%d)", kb->name, tokens[0], tmp_state, tokens[1], tmp_keyval);
    kb->state = tmp_state;
    kb->keyval = gdk_keyval_to_lower(tmp_keyval);
    g_strfreev(tokens);
}

static gint get_kb_index(const gchar* name)
{
    int i = 0;
    for (; i<configs.key_bindings->len; ++i)
    {
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, i);
        if (!strcmp(kb->name, name))
            return i;
    }
    return -1;
}

void termit_load_keybindings(GKeyFile* keyfile)
{
    const gchar* kb_group = "keybindings";
    GError * error = NULL;
    gsize len = 0;
    gchar **names = g_key_file_get_keys(keyfile, kb_group, &len, &error);
    
    int i = 0;
    for (; i<len; ++i)
    {
        gint index = get_kb_index(names[i]);
        if (index < 0)
            continue;
        gchar* value = g_key_file_get_value(keyfile, kb_group, names[i], &error);
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, index);
        set_keybinding(kb, value);
        g_free(value);
    }

    g_strfreev(names);
    TRACE_MSG("trace keybindings");
    trace_keybindings();
}

static gboolean termit_key_press_use_keycode(GdkEventKey *event)
{
    int i = 0;
    for (; i<configs.key_bindings->len; ++i)
    {
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, i);
        if (kb && (event->state & kb->state))
            if (event->hardware_keycode == kb->keycode)
            {
                kb->callback();
                return TRUE;
            }
    }
    return FALSE;
}

static gboolean termit_key_press_use_keysym(GdkEventKey *event)
{
    int i = 0;
    for (; i<configs.key_bindings->len; ++i)
    {
        struct KeyBindging* kb = &g_array_index(configs.key_bindings, struct KeyBindging, i);
        if (kb && (event->state & kb->state))
            if (gdk_keyval_to_lower(event->keyval) == kb->keyval)
            {
                kb->callback();
                return TRUE;
            }
    }
    return FALSE;
}

gboolean termit_process_key(GdkEventKey* event)
{
    switch(configs.kb_policy)
    {
    case TermitKbUseKeycode:
        return termit_key_press_use_keycode(event);
        break;
    case TermitKbUseKeysym:
        return termit_key_press_use_keysym(event);
        break;
    default:
        ERROR("unknown kb_policy");
    }
    return FALSE;
}

