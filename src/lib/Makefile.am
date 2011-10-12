MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EBIRD_CFLAGS@

lib_LTLIBRARIES = libebird.la

includes_HEADERS = Ebird.h ebird_private.h
includesdir = $(includedir)/ebird-@VMAJ@

libebird_la_SOURCES = ebird.c
libebird_la_LIBADD = @EBIRD_LIBS@
libebird_la_LDFLAGS = -no-undefined -version-info @version_info@

clean-local:
	rm -rf *~
