SUBDIRS = server client_libs examples rtk2 utils

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/player-manual && make manual
	cp -p $(srcdir)/player-manual/player.ps $(top_distdir)/doc

docdir = $(prefix)/doc
install-data-hook:
	mkdir -p $(top_srcdir)/doc
	cp -a --remove-destination $(top_srcdir)/doc $(DESTDIR)$(prefix) 

uninstall-local:
	rm -f $(DESTDIR)$(docdir)/*

