#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <X11/Xlib.h>
#include <gtk/gtk.h>

typedef void(*BindingCallback)();
struct KeyBindging
{
    gchar* name;
    guint state;
    guint keyval;
    KeySym keycode;
//    BindingCallback callback;
    int lua_callback;
    gchar* default_binding;
};

//void termit_load_keys();
void termit_set_default_keybindings();
gboolean termit_process_key(GdkEventKey* event);
#endif /* KEYBINDINGS_H */

