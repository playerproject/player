# only build if Python was found by configure
if WITH_PYTHON
noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall
endif

# use distutils to build
pythonbuild:
	swig -I../.. -python playerc.i
	python $(srcdir)/setup.py build

# files that go in distribution
EXTRA_DIST = playerc.i setup.py

clean-local:
	rm -f playerc_wrap.c playerc.py playerc.pyc
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	@PYTHON_INTERP@ $(srcdir)/setup.py install --prefix=$(prefix)

# seems that distutils doesn't support 'uninstall', oh well
uninstall-local:
	echo 'Uninstall not implemented (sorry)'

