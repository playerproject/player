tcllibdir = $(libdir)/tclPlayer
tcllib_DATA = playerclient.tcl pkgIndex.tcl

EXTRA_DIST = playerclient.tcl pkgIndex.tcl


pkgIndex.tcl: playerclient.tcl
	echo "pkg_mkIndex . playerclient.tcl" | tclsh

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/tclplayer.ps $(top_distdir)/doc
