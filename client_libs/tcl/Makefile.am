# The Tcl library is not installed, because it's way out of date
#tcllibdir = $(libdir)/tclPlayer
#tcllib_DATA = playerclient.tcl @TCLPKGFILE@

EXTRA_DIST = playerclient.tcl


pkgIndex.tcl: playerclient.tcl
	echo "pkg_mkIndex $(srcdir) $(srcdir)/playerclient.tcl" | @TCLSH@

# REMOVE; docs are no longer built in the distribution
#dist-hook:
#	mkdir -p $(top_distdir)/doc
#	cd $(srcdir)/doc && make manual
#	cp -p $(srcdir)/doc/tclplayer.ps $(top_distdir)/doc
