tcllibdir = $(libdir)/tclPlayer
tcllib_DATA = playerclient.tcl @TCLPKGFILE@

EXTRA_DIST = playerclient.tcl @TCLPKGFILE@


pkgIndex.tcl: playerclient.tcl
	echo "pkg_mkIndex . playerclient.tcl" | tclsh

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/tclplayer.ps $(top_distdir)/doc
