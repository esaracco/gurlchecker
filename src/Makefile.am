
AM_CFLAGS = \
	@GNUTLS_FLAGS@ @CLAMAV_FLAGS@ @TIDY_FLAGS@ @CROCO_FLAGS@ \
	@SQLITE3_FLAGS@ @JSON_FLAGS@ \
	-Wall -export-dynamic

CLEANFILES = *~

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
	-DPACKAGE_BIN_DIR=\""$(bindir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = gurlchecker

gurlchecker_SOURCES = \
	application.c application.h \
	bookmarks.c bookmarks.h \
	cache.c cache.h \
	callbacks.c callbacks.h \
	check.c check.h \
	connection.c connection.h \
	general.h \
	getopt1.c getopt.c getopt.h \
	html_parser.c html_parser.h \
	lists.c lists.h \
	main.c  \
	project.c project.h \
	report.c report.h \
	search.c search.h \
	timeout.c timeout.h \
	tooltips.c tooltips.h \
	ucclam.c ucclam.h \
	uccroco.c uccroco.h \
	uctidy.c uctidy.h \
	url.c url.h \
	utils.c utils.h \
	web_site.c web_site.h \
	cookies.c cookies.h

gurlchecker_LDADD = \
	@PACKAGE_LIBS@ \
	@GNUTLS_LIBS@ @CLAMAV_LIBS@ @TIDY_LIBS@ @CROCO_LIBS@ \
	@SQLITE3_LIBS@ @JSON_LIBS@\
	-lresolv
