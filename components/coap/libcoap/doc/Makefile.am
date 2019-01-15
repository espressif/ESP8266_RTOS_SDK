# doc/Makefile.am
#
# Copyright (C)      2015 Carsten Schoenert <c.schoenert@t-online.de>
#
# This file is part of the CoAP C library libcoap. Please see README and
# COPYING for terms of use.


# We can only perfom the targets in this directory if doxygen is present.

if HAVE_DOXYGEN

CLEANFILES = \
  doxygen_sqlite3.db


# This target has no check for automated install data! It's simply assumed that
# that the 'all' target is running before, in short: the user has to run first
# 'make' before running 'make install'!
install-data-am:
	@if [ ! -d $(top_builddir)/doc/html ]; then \
		echo ;\
		echo "     No install data in '$(top_builddir)/doc/html'found! Please run 'make all' first." ;\
		echo ;\
		exit 1 ;\
	fi
	$(MKDIR_P) $(DESTDIR)$(htmldir)/html || exit 1
	cp -a $(top_builddir)/doc/html $(DESTDIR)$(htmldir)
	find $(DESTDIR)$(htmldir) -type f -name "*.md5" -exec rm {} \;

uninstall-am:
	-rm -rf $(DESTDIR)$(htmldir)/html

all:
	$(DOXYGEN) Doxyfile

clean-local:
	rm -rf $(top_builddir)/doc/html

distclean-local: clean-local

endif # HAVE_DOXYGEN
