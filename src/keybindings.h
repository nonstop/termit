#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <gtk/gtk.h>

typedef void(*BindingCallback)();
struct KeyBindging
{
    gchar* name;
    guint state;
    guint keyval;
    BindingCallback callback;
    gchar* default_binding;
};

void termit_load_default_keybindings();
void termit_load_keybindings(GKeyFile* key_file);

#endif /* KEYBINDINGS_H */

