
if INCLUDE_LIBPLAYERC_PY

noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall

# files that must be built first
BUILT_SOURCES = playerc_oo.i

endif

# files that go in distribution
EXTRA_DIST = playerc.i setup.py parse.py

# Generate a OO interface file for libplayerc.  This is tweaked to
# unable VPAPTH (distcheck) builds; the header files needed by swig
# are copied into the build tree.
playerc_oo.i: ../../playerc.h $(top_srcdir)/server/player.h
	if test ! -s ../../playerc.h; then cp $(srcdir)/../../playerc.h ../../; fi
	if test ! -s $(top_builddir)/server/player.h; then cp $(top_srcdir)/server/player.h $(top_builddir)/server/; fi
	if test ! -s playerc.i; then cp $(srcdir)/playerc.i .; fi
	python $(srcdir)/parse.py ../../playerc.h playerc_oo.i

# Use distutils to build module
pythonbuild: playerc_oo.i ../../playerc.h
	python $(srcdir)/setup.py build

# For distcheck builds, clean up the files we created in the build tree
# This generates warnings, so I'm relying on the top-level Makefile.am override.
#distclean:
#	rm -f ../../playerc.h
#	rm -f $(top_builddir)/server/player.h
#	rm -f playerc_oo.i playerc.i playerc_wrap.c Makefile

# Normal cleanup
clean-local:
	rm -f playerc.py *.pyc playerc_oo.i
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	python $(srcdir)/setup.py install --prefix=$(prefix)

# seems that distutils doesn't support 'uninstall', oh well
uninstall-local:
	echo 'Uninstall not implemented (sorry)'

