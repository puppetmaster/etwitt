MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I$(includedir)


lib_LTLIBRARIES = libebird.la


libebird_SOURCES = \
	ebird.h \
	ebird.c
libebird_CPPFLAGS =          \
	@OAUTH_CFLAGS@     \
	@ECORE_CON_CFLAGS@ \
	@ECORE_FILE_CFLAGS@ \
	@OPENSSL_CFLAGS@ \
	@EET_CFLAGS@
libebird_LDADD =           \
	@OAUTH_LIBS@     \
	@ECORE_CON_LIBS@ \
	@ECORE_FILE_LIBS@  \
	@OPENSSL_LIBS@ \
	@EET_LIBS@

clean-local:
	rm -rf *~




MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-DPACKAGE_BIN_DIR=\"$(bindir)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@OAUTH_CFLAGS@     \
@ECORE_CON_CFLAGS@ \
@ECORE_FILE_CFLAGS@ \
@OPENSSL_CFLAGS@ \
@EET_CFLAGS@

includes_HEADERS = ebird.h
includesdir = $(includedir)/ebird-@VMAJ@

lib_LTLIBRARIES = libebird.la

base_sources = \
ebird.c

libebird_la_SOURCES = $(base_sources)

libebird_la_LIBADD =\
@OAUTH_LIBS@     \
@ECORE_CON_LIBS@ \
@ECORE_FILE_LIBS@  \
@OPENSSL_LIBS@ \
@EET_LIBS@


clean-local:
	@rm -rf *.gcno
