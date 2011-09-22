MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = ebird_test etwitt

etwitt_SOURCES = \
	etwitt.c

etwitt_CPPFLAGS = \
	-I$(top_srcdir)/src/lib \
	-I$(top_builddir)/src/lib \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR=\"$(datadir)\"\
	@ETWITT_CFLAGS@

etwitt_LDADD = \
	@ETWITT_LIBS@ \
        $(top_builddir)/src/lib/libebird.la

ebird_test_SOURCES = \
	ebird_test.c

ebird_test_CPPFLAGS = \
	-I$(top_srcdir)/src/lib \
	-I$(top_builddir)/src/lib \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR=\"$(datadir)\"\
	@EBIRD_TEST_CFLAGS@

ebird_test_LDADD = \
	@EBIRD_TEST_LIBS@ \
        $(top_builddir)/src/lib/libebird.la

clean-local:
	rm -rf *~
