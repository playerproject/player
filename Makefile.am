SUBDIRS = client_libs server utils config examples

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/player-manual && make manual
	cp -p $(srcdir)/player-manual/player.ps $(top_distdir)/doc

docdir = $(prefix)/doc
install-data-hook:
	mkdir -p $(DESTDIR)$(prefix)/doc
	cp $(top_srcdir)/doc/*.ps $(DESTDIR)$(prefix)/doc

uninstall-local:
	rm -f $(DESTDIR)$(docdir)/*

