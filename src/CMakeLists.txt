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

SET(TARGET "termit")

INCLUDE_DIRECTORIES(${TERMIT_SOURCE_DIR})

INCLUDE (FindGTK)

INCLUDE (FindPkgConfig)
pkg_search_module(VTE REQUIRED libvte>=0.17.1 vte>=0.17.1)
IF(NOT VTE_FOUND)
  message(FATAL_ERROR "vte library not found")
ENDIF(NOT VTE_FOUND)

INCLUDE(FindLua51)
IF(NOT LUA51_FOUND)
  message(FATAL_ERROR "lua 5.1 not found")
ENDIF(NOT LUA51_FOUND)

INCLUDE(Sources.cmake)

SET(TERMIT_CFLAGS "-I${LUA_INCLUDE_DIR}")
foreach(cflag ${VTE_CFLAGS} ${GTK_CFLAGS})
  set(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${cflag}")
endforeach(cflag)

IF(DEBUG)
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -g -D DEBUG")
ENDIF(DEBUG)

set(TERMIT_LDFLAGS ${VTE_LDFLAGS} ${GTK_LDFLAGS})

SET_SOURCE_FILES_PROPERTIES(${TERMIT_SRCS} PROPERTIES 
  COMPILE_FLAGS "${TERMIT_CFLAGS}")

ADD_EXECUTABLE(${TARGET} ${TERMIT_SRCS})

#set_target_properties(${TARGET} PROPERTIES 
#  LINK_FLAGS ${TERMIT_LDFLAGS})
#set_target_properties(${TARGET} PROPERTIES LINK_FLAGS ${TERMIT_LDFLAGS})

SET(TERMIT_LDFLAGS "")
FOREACH(ldflag ${VTE_LDFLAGS} ${GTK_LDFLAGS})
  SET(TERMIT_LDFLAGS "${TERMIT_LDFLAGS} ${ldflag}")
ENDFOREACH(ldflag)
set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "${TERMIT_LDFLAGS}")

target_link_libraries(${TARGET} ${LUA_LIBRARIES})

INSTALL(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

