## Process this file with automake to produce Makefile.in

# The name of the module, e.g. 'glib'.
DOC_MODULE=gurlchecker

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../src

# Extra options to pass to gtkdoc-scanobj or gtkdoc-scangobj.
SCANOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=
CFILE_GLOB=

# Header files to ignore when scanning.
IGNORE_HFILES= \
	getopt.h

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =

# Other files to distribute.
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS =
GTKDOC_LIBS =

GTKDOC_CC=$(LIBTOOL) --mode=compile $(CC)
GTKDOC_LD=$(LIBTOOL) --mode=link $(CC)

# If you need to override some of the declarations, place them in the
# $(DOC_MODULE)-overrides.txt file and uncomment the second line here.
DOC_OVERRIDES =
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt



###########################################################################
# Everything below here is generic and you shouldn't need to change it.
###########################################################################

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

EXTRA_DIST = 					\
	$(content_files)			\
	$(extra_files)				\
	$(HTML_IMAGES)				\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE)-sections.txt	\
	$(DOC_OVERRIDES)

DOC_STAMPS =					\
	scan-build.stamp			\
	tmpl-build.stamp			\
	sgml-build.stamp			\
	html-build.stamp			\
	$(srcdir)/tmpl.stamp		\
	$(srcdir)/sgml.stamp		\
	$(srcdir)/html.stamp

SCANOBJ_FILES =					\
	$(DOC_MODULE).args			\
	$(DOC_MODULE).hierarchy		\
	$(DOC_MODULE).interfaces	\
	$(DOC_MODULE).prerequisites	\
	$(DOC_MODULE).signals

CLEANFILES = $(SCANOBJ_FILES) $(DOC_MODULE)-unused.txt $(DOC_STAMPS)

MAINTAINERCLEANFILES = Makefile.in

if ENABLE_GTK_DOC
all-local: html-build.stamp

#### scan ####

scan-build.stamp: $(HFILE_GLOB)
	@echo '*** Scanning header files ***'
	if grep -l '^..*$$' $(srcdir)/$(DOC_MODULE).types > /dev/null;	\
	then													\
		CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)"					\
		CFLAGS="$(GTKDOC_CFLAGS)" LDFLAGS="$(GTKDOC_LIBS)"	\
		gtkdoc-scanobj										\
			$(SCANOBJ_OPTIONS)								\
			--module=$(DOC_MODULE)							\
			--output-dir=$(srcdir) ;						\
	else													\
	    cd $(srcdir) ;										\
		chmod u+w . ;										\
		for i in $(SCANOBJ_FILES) ; do						\
			test -f $$i || touch $$i ;						\
		done												\
	fi
	cd $(srcdir) &&											\
	gtkdoc-scan												\
		$(SCAN_OPTIONS) $(EXTRA_HFILES)						\
		--module=$(DOC_MODULE)								\
		--source-dir=$(DOC_SOURCE_DIR)						\
		--ignore-headers="$(IGNORE_HFILES)"
	touch scan-build.stamp

$(DOC_MODULE)-decl.txt $(SCANOBJ_FILES): scan-build.stamp
	@true

#### templates ####

tmpl-build.stamp: $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt $(DOC_OVERRIDES)
	@echo '*** Rebuilding template files ***'
	chmod u+w ${srcdir}/tmpl
	cd $(srcdir) && \
	gtkdoc-mktmpl --module=$(DOC_MODULE) | tee tmpl-build.log

	@cd $(srcdir) && \
	if grep "WARNING:" tmpl-build.log > /dev/null; then \
	    cat $(DOC_MODULE)-unused.txt ; \
	    exit 1; fi
	cd $(srcdir) && \
	rm -f tmpl-build.log
	touch tmpl-build.stamp

tmpl.stamp: tmpl-build.stamp
	@true

#### xml ####

sgml-build.stamp: tmpl.stamp $(CFILE_GLOB)
	@echo '*** Building XML ***'
	test -d $(srcdir)/xml || mkdir $(srcdir)/xml
	chmod u+w $(srcdir)/xml
	cd $(srcdir) &&								\
	gtkdoc-mkdb									\
		--module=$(DOC_MODULE)					\
		--source-dir=$(DOC_SOURCE_DIR)			\
		--main-sgml-file=$(DOC_MAIN_SGML_FILE)	\
		--output-format=xml						\
		$(MKDB_OPTIONS)
# | tee xml-build.log
#	@if grep "WARNING:" sgml-build.log > /dev/null; then exit 1; fi
#	rm sgml-build.log
	touch sgml-build.stamp

sgml.stamp: sgml-build.stamp
	@true

#### html ####

html-build.stamp: sgml.stamp $(DOC_MAIN_SGML_FILE) $(content_files)
	@echo '*** Building HTML ***'
	if test -d $(srcdir)/html; then chmod u+w $(srcdir)/html ; rm -rf $(srcdir)/html; fi
	mkdir $(srcdir)/html
	chmod u+w $(srcdir)/html
	cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)

	test "x$(HTML_IMAGES)" = "x" || for i in "" $(HTML_IMAGES) ; do \
	    if test "$$i" != ""; then cp $(srcdir)/$$i html ; fi; done

	@echo '-- Fixing Crossreferences' 
	cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)

	touch html-build.stamp
else
all-local:
endif

##############

clean-local:
	rm -f *~ *.bak tmpl/*.bak
if ENABLE_GTK_DOC
	rm -rf xml html
endif
	rm -f $(SCANOBJ_FILES) *-unused.txt $(DOC_STAMPS)

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf xml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

distclean-local: clean
	rm -rf $(DOC_MODULE)-decl-list.txt
	rm -rf $(DOC_MODULE)-decl.txt
	rm -rf $(DOC_MODULE)-undocumented.txt
	rm -rf $(DOC_MODULE)-unused.txt
	rm -rf tmpl/*.sgml.bak
	rm -f $(DOC_MODULE).hierarchy
	rm -f *.stamp || true
	if test x"$(srcdir)" != x. ; then \
	    rm -f $(DOC_MODULE)-docs.sgml ; \
	    rm -f $(DOC_MODULE).types ; \
	    rm -f $(DOC_MODULE).interfaces ; \
	    rm -f $(DOC_MODULE).prerequisites ; \
	    rm -f $(DOC_MODULE)-sections.txt ; \
	    rm -rf tmpl/*.sgml ; \
	fi
	rm -rf *.o

if ENABLE_GTK_DOC
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  pngfiles=`echo ./html/*.png`; \
	  if test "$$pngfiles" != './html/*.png'; then \
	    for i in $$pngfiles; do \
	      echo '-- Installing '$$i ; \
	      $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	    done; \
	  fi; \
	  echo '-- Installing $(srcdir)/html/$(DOC_MODULE).devhelp' ; \
	  $(INSTALL_DATA) $(srcdir)/html/$(DOC_MODULE).devhelp \
	    $(DESTDIR)$(TARGET_DIR)/$(DOC_MODULE).devhelp; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	fi)
uninstall-local:
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to uninstall' ; \
	else \
	  for i in $$installfiles; do \
	    rmfile=`basename $$i` ; \
	    echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/'$$rmfile ; \
	    rm -f $(DESTDIR)$(TARGET_DIR)/$$rmfile; \
	  done; \
	  pngfiles=`echo $(srcdir)/html/*.png`; \
	  if test "$$pngfiles" != '$(srcdir)/html/*.png'; then \
	    for i in $$pngfiles; do \
	      rmfile=`basename $$i` ; \
	      echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/'$$rmfile ; \
	      rm -f $(DESTDIR)$(TARGET_DIR)/$$rmfile; \
	    done; \
	  fi; \
	  echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/$(DOC_MODULE).devhelp' ; \
	  rm -f $(DESTDIR)$(TARGET_DIR)/$(DOC_MODULE).devhelp; \
	  echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/index.sgml' ; \
	  rm -f $(DESTDIR)$(TARGET_DIR)/index.sgml; \
	fi) 
	if test -d $(DESTDIR)$(TARGET_DIR); then rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(TARGET_DIR) 2>/dev/null; fi

#
# Require gtk-doc when making dist
#
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc must be installed and enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtkdoc dist-hook-local
	mkdir $(distdir)/tmpl
	mkdir $(distdir)/xml
	mkdir $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/xml/*.xml $(distdir)/xml
	-cp $(srcdir)/html/*.html $(distdir)/html
	-cp $(srcdir)/html/*.png $(distdir)/html
	-cp $(srcdir)/html/gurlchecker.devhelp $(distdir)/html

	images=$(HTML_IMAGES) ;    	      \
	for i in "" $$images ; do		      \
	  if test "$$i" != ""; then cp $(srcdir)/$$i $(distdir)/html ; fi; \
	done

.PHONY : dist-hook-local
