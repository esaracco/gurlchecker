#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gurlchecker"

(test -f $srcdir/configure.in) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}

REQUIRED_AUTOMAKE_VERSION=1.8 \
REQUIRED_AUTOCONF_VERSION=2.50 \
REQUIRED_LIBTOOL_VERSION=1.5 \
REQUIRED_GTK_DOC_VERSION=1.1 \
USE_GNOME2_MACROS=1 . gnome-autogen.sh
