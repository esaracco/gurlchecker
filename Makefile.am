## Process this file with automake to produce Makefile.in

SUBDIRS = src stylesheets po doc ui

man1_MANS = gurlchecker.1

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	$(DESKTOP_IN_FILES) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(PACKAGE).1 \
	FAQ \
	CONTRIBUTORS

CLEANFILES = *~

DISTCLEANFILES = \
	$(DESKTOP_DATA) \
	intltool-extract \
	intltool-update \
	intltool-merge

MAINTAINERCLEANFILES = \
	aclocal.m4 \
	config.h.in \
	config.guess \
	config.sub \
	depcomp \
	configure \
	install-sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	ltmain.sh \
	missing \
	mkinstalldirs \
	Makefile.in \
	$(PACKAGE)-*.tar.gz

DESKTOP_IN_FILES = gurlchecker.desktop.in

@INTLTOOL_DESKTOP_RULE@

DESKTOPdir = $(datadir)/applications
DESKTOP_DATA = $(DESKTOP_IN_FILES:.desktop.in=.desktop)

licensedir = $(datadir)/doc/$(PACKAGE)
license_DATA = COPYING
