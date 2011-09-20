MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I$(top_srcdir)/src/lib \
	   -I$(includedir)

bin_PROGRAMS = ebird_test etwitt

etwitt_SOURCES = \
	etwitt.c

etwitt_CPPFLAGS = \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR=\"$(datadir)\"\
	@ELM_CFLAGS@

etwitt_LDADD = \
	@ELM_LIBS@ \
        $(top_builddir)/src/lib/libebird.la

ebird_test_SOURCES = \
	ebird_test.c

ebird_test_CPPFLAGS = \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR=\"$(datadir)\"\
	@OAUTH_CFLAGS@     \
	@ECORE_CON_CFLAGS@ \
	@ECORE_FILE_CFLAGS@ \
	@OPENSSL_CFLAGS@ \
	@EET_CFLAGS@

ebird_test_LDADD =           \
	@OAUTH_LIBS@     \
	@ECORE_CON_LIBS@ \
	@ECORE_FILE_LIBS@  \
	@OPENSSL_LIBS@ \
        @EET_LIBS@ \
        $(top_builddir)/src/lib/libebird.la

clean-local:
	rm -rf *~
