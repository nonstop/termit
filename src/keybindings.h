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
void termit_bind_key(const gchar* keys, int lua_callback);
void termit_unbind_key(const gchar* keys);
void termit_bind_mouse(const gchar* mouse_event, int lua_callback);
void termit_unbind_mouse(const gchar* mouse_event);
void termit_set_default_keybindings();
gboolean termit_key_event(GdkEventKey* event);
gboolean termit_mouse_event(GdkEventButton* event);
#endif /* KEYBINDINGS_H */

