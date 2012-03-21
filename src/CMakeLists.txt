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
pkg_search_module(VTE REQUIRED libvte>=0.17 vte>=0.17)
IF(NOT VTE_FOUND)
  message(FATAL_ERROR "vte library not found")
ENDIF(NOT VTE_FOUND)

SET(LUA_INCLUDE_DIR "/usr/include" CACHE STRING "Lua include dir")
SET(LUA_LIBRARIES "/usr/lib/liblua.a" CACHE STRING "Lua libraries")

IF(NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARIES)
  INCLUDE(FindLua51)
  IF(NOT LUA51_FOUND)
    message(FATAL_ERROR "lua 5.1 not found")
  ENDIF(NOT LUA51_FOUND)
ENDIF(NOT LUA_INCLUDE_DIR OR NOT LUA_LIBRARIES)

INCLUDE(Sources.cmake)

SET(TERMIT_CFLAGS "-I${LUA_INCLUDE_DIR}")
FOREACH(cflag ${VTE_CFLAGS} ${GTK_CFLAGS})
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${cflag}")
ENDFOREACH(cflag)

IF(NOT ${VTE_VERSION} LESS "0.26")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -DTERMIT_ENABLE_SEARCH")
ENDIF(NOT ${VTE_VERSION} LESS "0.26")

IF(DEBUG)
  SET(EXTRA_CFLAGS "-Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${EXTRA_CFLAGS} -g -D DEBUG -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE")
ELSE(DEBUG)
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -O2")
ENDIF(DEBUG)

set(TERMIT_LDFLAGS ${VTE_LDFLAGS} ${GTK_LDFLAGS})

SET_SOURCE_FILES_PROPERTIES(${TERMIT_SRCS} PROPERTIES 
  COMPILE_FLAGS "${TERMIT_CFLAGS}")

ADD_EXECUTABLE(${TARGET} ${TERMIT_SRCS})

SET(TERMIT_LDFLAGS "")
FOREACH(ldflag ${VTE_LDFLAGS} ${GTK_LDFLAGS})
  LIST(APPEND TERMIT_LDFLAGS "${ldflag}")
ENDFOREACH(ldflag)

target_link_libraries(${TARGET} ${TERMIT_LDFLAGS} ${LUA_LIBRARIES})

INSTALL(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

