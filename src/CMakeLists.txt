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

INCLUDE (FindPkgConfig)

SET(VTE_VERSION "" CACHE STRING "Vte version")
IF(VTE_VERSION STREQUAL "")
  pkg_search_module(VTE REQUIRED vte-2.90 libvte>=0.17 vte>=0.17)
ELSE(VTE_VERSION STREQUAL "")
  pkg_search_module(VTE REQUIRED vte-2.90=${VTE_VERSION} libvte=${VTE_VERSION} vte=${VTE_VERSION})
ENDIF(VTE_VERSION STREQUAL "")
IF(NOT VTE_FOUND)
  message(FATAL_ERROR "Vte library was not found.")
ENDIF(NOT VTE_FOUND)

SET(GTK_VERSION "" CACHE STRING "Gtk version")
IF(VTE_VERSION GREATER "0.29" OR VTE_VERSION EQUAL "0.29")
  IF(GTK_VERSION STREQUAL "")
    pkg_search_module(GTK REQUIRED gtk+-3.0)
  ELSE(GTK_VERSION STREQUAL "")
    pkg_search_module(GTK REQUIRED gtk+-3.0=${GTK_VERSION})
  ENDIF(GTK_VERSION STREQUAL "")
  IF(NOT GTK_FOUND)
    message(FATAL_ERROR "Gtk+-3.0 library was not found.")
  ENDIF(NOT GTK_FOUND)
ELSE(VTE_VERSION GREATER "0.29" OR VTE_VERSION EQUAL "0.29")
  IF(GTK_VERSION STREQUAL "")
    pkg_search_module(GTK REQUIRED gtk+-2.0>=2.12)
  ELSE(GTK_VERSION STREQUAL "")
    pkg_search_module(GTK REQUIRED gtk+-2.0=${GTK_VERSION})
  ENDIF(GTK_VERSION STREQUAL "")
  IF(NOT GTK_FOUND)
    message(FATAL_ERROR "Gtk+-2.0 library was not found.")
  ENDIF(NOT GTK_FOUND)
ENDIF(VTE_VERSION GREATER "0.29" OR VTE_VERSION EQUAL "0.29")

SET(LUA_VERSION "" CACHE STRING "Lua version")
IF(LUA_VERSION STREQUAL "")
  pkg_search_module(LUA REQUIRED lua>=5.0 lua5.2 lua5.1)
ELSE(LUA_VERSION STREQUAL "")
  pkg_search_module(LUA REQUIRED
    lua=${LUA_VERSION} lua5.2=${LUA_VERSION} lua5.1=${LUA_VERSION} lua5.0=${LUA_VERSION})
ENDIF(LUA_VERSION STREQUAL "")
IF(NOT LUA_FOUND)
  message(FATAL_ERROR "Lua library was not found.")
ENDIF(NOT LUA_FOUND)

INCLUDE(Sources.cmake)

SET(TERMIT_CFLAGS "-I${LUA_INCLUDE_DIR}")
FOREACH(cflag ${VTE_CFLAGS} ${GTK_CFLAGS} ${LUA_CFLAGS})
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${cflag}")
ENDFOREACH(cflag)

IF(NOT ${VTE_VERSION} LESS "0.26")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -DTERMIT_ENABLE_SEARCH")
ENDIF(NOT ${VTE_VERSION} LESS "0.26")

IF(DEBUG)
  SET(EXTRA_CFLAGS "-Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter")
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} ${EXTRA_CFLAGS} -g -DDEBUG -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE")
ELSE(DEBUG)
  SET(TERMIT_CFLAGS "${TERMIT_CFLAGS} -O2")
ENDIF(DEBUG)

set(TERMIT_LDFLAGS ${VTE_LDFLAGS} ${GTK_LDFLAGS})

SET_SOURCE_FILES_PROPERTIES(${TERMIT_SRCS} PROPERTIES 
  COMPILE_FLAGS "${TERMIT_CFLAGS}")

ADD_EXECUTABLE(${TARGET} ${TERMIT_SRCS})

SET(TERMIT_LDFLAGS "-ldl")
FOREACH(ldflag ${VTE_LDFLAGS} ${GTK_LDFLAGS} ${LUA_LDFLAGS})
  LIST(APPEND TERMIT_LDFLAGS "${ldflag}")
ENDFOREACH(ldflag)

target_link_libraries(${TARGET} ${TERMIT_LDFLAGS})

INSTALL(TARGETS ${TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)