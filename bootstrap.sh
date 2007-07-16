#!/bin/sh

gettextize --copy --force --intl
aclocal -I m4
libtoolize --force --copy
autoheader
autoconf
automake --add-missing --copy

