
if INCLUDE_LIBPLAYERC_PY

noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall

# files that must be built first
BUILT_SOURCES = playerc_oo.i

endif

python_builddir=$(top_builddir)/client_libs/libplayerc/bindings/python

# files that go in distribution
EXTRA_DIST = playerc.i setup.py parse.py

# Generate a OO interface file for libplayerc
playerc_oo.i: ../../playerc.h
	cd $(srcdir) && python parse.py ../../playerc.h playerc_oo.i && touch playerc.i && cd $(python_builddir)

# Use distutils to build.  Note the trick here to enable VPATH (and
# distcheck) builds: we jump into the source tree to build the module,
# but set the output path to point to the build tree.
pythonbuild: playerc_oo.i
	cd $(srcdir) && python setup.py build --build-temp=$(python_builddir)/build && cd $(python_builddir)

clean-local:
	rm -f playerc.py *.pyc playerc_oo.i
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	cd $(srcdir) && python $(srcdir)/setup.py install --prefix=$(prefix) && cd $(python_builddir)

# seems that distutils doesn't support 'uninstall', oh well
uninstall-local:
	echo 'Uninstall not implemented (sorry)'

