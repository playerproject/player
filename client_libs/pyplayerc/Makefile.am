# the name of the shared object to be built
PYPLAYERC_LIB=playerc.so
# the path to where Python will put the shared object; don't know how portable
# this hackery is
PYPLAYERC_LIBPATH=build/lib.$(shell uname -s | tr -s [:upper:] [:lower:])-$(shell uname -m)-@PYTHON_VERSION@/$(PYPLAYERC_LIB)

# only build if Python was found by configure
if WITH_PYTHON
noinst_DATA = $(PYPLAYERC_LIBPATH)
else
noinst_DATA =
endif

# use distutils to build
$(PYPLAYERC_LIBPATH): dev_blobfinder.c dev_fiducial.c dev_laser.c \
                      dev_position.c dev_ptz.c pyplayerc.c pyplayerc.h setup.py
	python setup.py build


EXTRA_DIST = dev_blobfinder.c \
             dev_fiducial.c \
             dev_laser.c \
             dev_position.c \
             dev_ptz.c \
             pyplayerc.c \
             pyplayerc.h \
	     setup.py

clean-local:
	rm -rf build

# use distutils to install
install-data-hook:
	python setup.py install --install-platlib=$(libdir)/python

# seems that distutils doesn't support 'uninstall', so we'll hack it manually
uninstall-local:
	rm -rf $(libdir)/python

