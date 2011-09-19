MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I$(includedir)

bin_PROGRAMS = ebird


ebird_SOURCES = \
	ebird.h \
	ebird.c
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
