#!/bin/sh

gettextize --copy --force --intl
aclocal
libtoolize --force --copy
autoheader
autoconf
automake --add-missing --copy

