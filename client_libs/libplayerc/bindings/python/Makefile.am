
if INCLUDE_LIBPLAYERC_PY
noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall
endif

# files that go in distribution
EXTRA_DIST = playerc.i libplayerc.py setup.py

# files that must be built first
BUILT_SOURCES = playerc_wrap.c

# use this target to update the bindings
playerc_wrap.c: playerc.i
	swig -I../.. -I$(top_srcdir)/server -python -dascii playerc.i

# use distutils to build
pythonbuild: playerc_wrap.c
	python $(srcdir)/setup.py build

clean-local:
	rm -f playerc_wrap.c libplayerc.py *.pyc
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	@PYTHON_INTERP@ $(srcdir)/setup.py install --prefix=$(prefix)

# seems that distutils doesn't support 'uninstall', oh well
uninstall-local:
	echo 'Uninstall not implemented (sorry)'

