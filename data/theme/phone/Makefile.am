MAINTAINERCLEANFILES = Makefile.in

EDJE_FLAGS = \
	-id $(top_srcdir)/data/images 

phonedir = $(pkgdatadir)/theme

phone_DATA = phone.edj

EDC = \
phone_new.edc \
widgets/*.edc

IMGS = \
$(top_srcdir)/data/images/*.png 

EXTRA_DIST = \
	$(EDC) \
	$(IMGS)

phone.edj: Makefile $(EXTRA_DIST)
	$(AM_V_GEN)@edje_cc@ $(EDJE_FLAGS) \
	$(top_srcdir)/data/theme/phone/phone_new.edc \
	$(top_builddir)/data/theme/phone/phone.edj

clean-local:
	rm -f *.edj
