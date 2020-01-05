# Copyright © 2007-2016 Evgeny Ratnikov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SET(TERMIT_DOCS ../COPYING README lua_api.txt rc.lua.example)

INSTALL(FILES ${TERMIT_DOCS} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/${TERMIT_PACKAGE})
INSTALL(FILES termit.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
INSTALL(FILES termit.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
INSTALL(FILES termit.metainfo.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo)

