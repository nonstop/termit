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

#ifndef TERMIT_KEYBINDINGS_H
#define TERMIT_KEYBINDINGS_H

#include <X11/Xlib.h>
#include <gtk/gtk.h>

struct KeyWithState
{
    guint state;
    guint keyval;
};

typedef void(*BindingCallback)();
struct KeyBinding
{
    gchar* name;
    struct KeyWithState kws;
    KeySym keycode;
    int lua_callback;
};

struct MouseBinding
{
    GdkEventType type;
    int lua_callback;
};

//void termit_load_keys();
int termit_parse_keys_str(const gchar* keybinding, struct KeyWithState* kws);
void termit_keys_bind(const gchar* keys, int lua_callback);
void termit_keys_unbind(const gchar* keys);
void termit_mouse_bind(const gchar* mouse_event, int lua_callback);
void termit_mouse_unbind(const gchar* mouse_event);
void termit_keys_set_defaults();
gboolean termit_key_event(GdkEventKey* event);
gboolean termit_mouse_event(GdkEventButton* event);
#endif /* TERMIT_KEYBINDINGS_H */
