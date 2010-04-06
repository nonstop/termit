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

SET( TERMIT_SRCS
   termit.c termit_core_api.c callbacks.c sessions.c
   keybindings.c lua_conf.c lua_api.c configs.c
   termit_style.c termit_preferences.c
)
SET( TERMIT_HDRS
    termit_core_api.h callbacks.h configs.h sessions.h
    keybindings.h lua_api.h termit.h termit_style.h
)
