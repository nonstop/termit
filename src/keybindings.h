#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <X11/Xlib.h>
#include <gtk/gtk.h>

typedef void(*BindingCallback)();
struct KeyBinding
{
    gchar* name;
    guint state;
    guint keyval;
    KeySym keycode;
    int lua_callback;
};

struct MouseBinding
{
    GdkEventType type;
    int lua_callback;
};

//void termit_load_keys();
void termit_keys_bind(const gchar* keys, int lua_callback);
void termit_keys_unbind(const gchar* keys);
void termit_mouse_bind(const gchar* mouse_event, int lua_callback);
void termit_mouse_unbind(const gchar* mouse_event);
void termit_keys_set_defaults();
gboolean termit_key_event(GdkEventKey* event);
gboolean termit_mouse_event(GdkEventButton* event);
#endif /* KEYBINDINGS_H */

