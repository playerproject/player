SUBDIRS = replace client_libs server utils config examples

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/player-manual && make manual
	cp -p $(srcdir)/player-manual/player.ps $(top_distdir)/doc

#docdir = $(prefix)/share/doc
install-data-hook:
	mkdir -p $(DESTDIR)$(prefix)/share/doc
	cp $(top_srcdir)/doc/*.ps $(DESTDIR)$(prefix)/share/doc
#cp $(top_srcdir)/server/drivers/localization/mcl/*.nb $(DESTDIR)$(prefix)/share/doc

# need to figure out a safe way to do this
#uninstall-local:
#	rm -f $(DESTDIR)$(docdir)/*

