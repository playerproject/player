
if WITH_RTK3
SUBDIRS = replace rtk3 client_libs server utils config examples
else
SUBDIRS = replace client_libs server utils config examples
endif

EXTRA_DIST = config.simulation config.p2os

dist-hook:
	mkdir -p $(top_distdir)/doc
	cp -p $(srcdir)/player-manual/player.1 $(top_distdir)/doc
	cp -p $(srcdir)/player-manual/README $(top_distdir)/doc

#cd $(srcdir)/player-manual && make manual
#cp -p $(srcdir)/player-manual/player.ps $(top_distdir)/doc


#docdir = $(prefix)/share/doc
install-data-hook:
	mkdir -p $(DESTDIR)$(prefix)/share/doc
	cp $(top_srcdir)/doc/*.ps $(DESTDIR)$(prefix)/share/doc
	cp $(top_srcdir)/doc/README $(DESTDIR)$(prefix)/share/doc/README.player
	mkdir -p $(DESTDIR)$(prefix)/man/man1
	cp $(top_srcdir)/doc/player.1 $(DESTDIR)$(prefix)/man/man1

#cp $(top_srcdir)/server/drivers/localization/mcl/*.nb $(DESTDIR)$(prefix)/share/doc

# need to figure out a safe way to do this
#uninstall-local:
#	rm -f $(DESTDIR)$(docdir)/*

