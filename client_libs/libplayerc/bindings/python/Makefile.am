
if INCLUDE_LIBPLAYERC_PY

noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall

# files that must be built first
BUILT_SOURCES = playerc_oo.i

endif

# files that go in distribution
EXTRA_DIST = playerc.i setup.py parse.py

# Generate a OO interface file for libplayerc
playerc_oo.i: ../../playerc.h
	python parse.py ../../playerc.h > playerc_oo.i
	touch playerc.i

# use distutils to build
pythonbuild:
	python $(srcdir)/setup.py build

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

