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

SET(TARGET "termit")

INCLUDE_DIRECTORIES(${TERMIT_SOURCE_DIR})

INCLUDE (FindPkgConfig)

pkg_search_module(GTK REQUIRED gtk+-3.0>=3.22)
pkg_search_module(VTE REQUIRED libvte>=0.46 vte>=0.46 vte-2.91>=0.46)
IF(NOT VTE_FOUND)
  message(FATAL_ERROR "Vte library was not found.")
ENDIF(NOT VTE_FOUND)

SET(LUA_VERSION "" CACHE STRING "Lua version")
IF(LUA_VERSION STREQUAL "")
    IF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
       pkg_search_module(LUA REQUIRED lua53)
    ELSE(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
       pkg_search_module(LUA REQUIRED lua>=5.3 lua5.3)
    ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
ELSE(LUA_VERSION STREQUAL "")
  pkg_search_module(LUA REQUIRED
    lua=${LUA_VERSION} lua5.3=${LUA_VERSION})
ENDIF(LUA_VERSION STREQUAL "")
IF(NOT LUA_FOUND)
  message(FATAL_ERROR "Lua library was not found.")
ENDIF(NOT LUA_FOUND)

INCLUDE(Sources.cmake)

SET(TERMIT_CFLAGS "-I${LUA_INCLUDE_DIR}")
IF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -I/usr/local/include")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
FOREACH(cflag ${VTE_CFLAGS} ${GTK_CFLAGS} ${LUA_CFLAGS})
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${cflag}")
ENDFOREACH(cflag)

IF(DEBUG)
  SET(EXTRA_CFLAGS "-std=c99 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${EXTRA_CFLAGS} -g -DDEBUG -DGTK_DISABLE_DEPRECATED")
ELSE(DEBUG)
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -O2")
ENDIF(DEBUG)

set(TERMIT_LDFLAGS ${VTE_LDFLAGS} ${GTK_LDFLAGS})

SET_SOURCE_FILES_PROPERTIES(${TERMIT_SRCS} PROPERTIES 
  COMPILE_FLAGS "${TERMIT_CFLAGS}")

ADD_EXECUTABLE(${TARGET} ${TERMIT_SRCS})

IF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    SET(TERMIT_LDFLAGS"-llua5.3")
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    SET(TERMIT_LDFLAGS "-ldl -lm")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
FOREACH(ldflag ${VTE_LDFLAGS} ${LUA_LDFLAGS})
  LIST(APPEND TERMIT_LDFLAGS "${ldflag}")
ENDFOREACH(ldflag)
LIST(APPEND TERMIT_LDFLAGS "-lX11")

target_link_libraries(${TARGET} ${TERMIT_LDFLAGS})

INSTALL(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

