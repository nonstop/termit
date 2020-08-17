/* Copyright © 2020 Nimrod Maclomair
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

#ifndef TERMIT_EVENTBINDINGS_H
#define TERMIT_EVENTBINDINGS_H

struct EventBinding
{
    gchar* name;
    int lua_callback;
};


void termit_event_bind(const gchar* event, int lua_callback);
void termit_event_unbind(const gchar* event);
gboolean termit_event(const gchar* event);


#endif

