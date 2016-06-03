# Copyright 1999-2008 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=2

CMAKE_MIN_VERSION="2.6.1"

inherit cmake-utils

SRC_URI="https://github.com/nonstop/${PN}/archive/${PV}.tar.gz -> ${P}.tar.gz"
HOMEPAGE="http://wiki.github.com/nonstop/termit/"
DESCRIPTION="Simple terminal emulator based on vte library with Lua scripting."

RDEPEND=">=x11-libs/vte-0.42.5
    >=x11-libs/gtk+-3.18
    >=dev-lang/lua-5.2"
DEPEND="${RDEPEND}"

SLOT="0"
LICENSE="GPL-3"
KEYWORDS="~x86 ~amd64"
IUSE=""

DOCS="INSTALL ChangeLog doc/README doc/rc.lua.example doc/lua_api.txt"

CMAKE_IN_SOURCE_BUILD="yes"

pkg_postinst() {
	einfo
	einfo "There is a example of configfile in "
	einfo "		/usr/share/doc/termit/rc.lua.example "
	einfo "Copy this file to "
	einfo "		\$HOME/.config/termit/rc.lua"
	einfo "and modify to fit your needs."
	einfo
}
