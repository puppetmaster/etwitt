AUTOMAKE_OPTIONS     = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

EDJE_FLAGS = -v \
-id $(top_srcdir)/data/images

filesdir = @datadir_edj@
files_DATA = phone.edj

EXTRA_DIST = phone.edc

phone.edj:  phone.edc
	$(EDJE_CC) $(EDJE_FLAGS) $< $@

clean-local:
	 rm -rf phone.edj *~

uninstall:
	 rm -rf $(DESTDIR)@datadir_edj@/phone.edj
