MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I$(includedir)

bin_PROGRAMS = ebird etwitt

etwitt_SOURCES = \
	etwitt.c
etwitt_CPPFLAGS = \
	@ELM_CFLAGS@
etwitt_LDADD = \
	@ELM_LIBS@

ebird_SOURCES = \
	ebird.c
ebird_CPPFLAGS =          \
	@OAUTH_CFLAGS@     \
	@ECORE_CON_CFLAGS@ \
	@OPENSSL_CFLAGS@ \
    @EET_CFLAGS@
ebird_LDADD =           \
	@OAUTH_LIBS@     \
	@ECORE_CON_LIBS@ \
	@OPENSSL_LIBS@ \
    @EET_LIBS@

clean-local:
	rm -rf *~
