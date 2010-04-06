# Copyright (C) 2007-2010, Evgeny Ratnikov
#
# This file is part of termit.
# termit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 
# as published by the Free Software Foundation.
# termit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with termit. If not, see <http://www.gnu.org/licenses/>.

SET(TERMIT_DOCS ../COPYING README lua_api.txt rc.lua.example)

INSTALL(FILES ${TERMIT_DOCS} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/${TERMIT_PACKAGE})
INSTALL(FILES termit.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
INSTALL(FILES termit.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)

