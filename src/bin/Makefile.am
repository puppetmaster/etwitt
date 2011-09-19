MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I../lib \
	   -I$(includedir)

bin_PROGRAMS = ebird etwitt

etwitt_SOURCES = \
	etwitt.c
etwitt_CPPFLAGS = \
	@ELM_CFLAGS@
etwitt_LDADD = \
	@ELM_LIBS@

ebird_SOURCES = \
	ebird_test.c
ebird_CPPFLAGS =          \
	@OAUTH_CFLAGS@     \
	@ECORE_CON_CFLAGS@ \
	@ECORE_FILE_CFLAGS@ \
	@OPENSSL_CFLAGS@ \
    @EET_CFLAGS@
ebird_LDADD =           \
	@OAUTH_LIBS@     \
	@ECORE_CON_LIBS@ \
	@ECORE_FILE_LIBS@  \
	@OPENSSL_LIBS@ \
    @EET_LIBS@

clean-local:
	rm -rf *~
