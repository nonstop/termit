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

//void termit_load_keys();
void termit_bind_key(const gchar* keys, int lua_callback);
void termit_unbind_key(const gchar* keys);
void termit_set_default_keybindings();
gboolean termit_process_key(GdkEventKey* event);
#endif /* KEYBINDINGS_H */

