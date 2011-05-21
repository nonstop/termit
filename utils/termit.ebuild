# Copyright 1999-2008 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit cmake-utils

SRC_URI="http://github.com/downloads/nonstop/termit/${P}.tar.bz2"
HOMEPAGE="http://wiki.github.com/nonstop/termit/"
DESCRIPTION="Simple terminal emulator based on vte library with Lua scripting"

RDEPEND="x11-libs/vte
    >=x11-libs/gtk+-2.10
    >=dev-lang/lua-5.1"
DEPEND="${RDEPEND}
        >=dev-util/cmake-2.4"

SLOT="0"
LICENSE="GPL-2"
KEYWORDS="~x86"
IUSE=""

DOCS="README ChangeLog doc/rc.lua.example doc/lua_api.txt"

CMAKE_IN_SOURCE_BUILD="yes"

pkg_postinst() {
	einfo
	einfo "There is a example of configfile in "
	einfo "		/usr/share/doc/termit/rc.lua.example "
	einfo "copy this file to "
	einfo "		\$HOME/.config/termit/rc.lua"
	einfo "and modify to fit your needs "
	einfo
}

