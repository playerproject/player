
if INCLUDE_LIBPLAYERC_PY

noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall

endif

# files that go in distribution
EXTRA_DIST = playerc.i setup.py

# Generate a OO interface file for libplayerc
#playerc_oo.i: ../../playerc.h
#	python parse.py ../../playerc.h > playerc_oo.i

# use distutils to build
pythonbuild:
	python $(srcdir)/setup.py build

clean-local:
	rm -f playerc.py *.pyc
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	python $(srcdir)/setup.py install --prefix=$(prefix)

# seems that distutils doesn't support 'uninstall', oh well
uninstall-local:
	echo 'Uninstall not implemented (sorry)'

