# only build if Python was found by configure
if WITH_PYTHON
noinst_DATA = pythonbuild
PYTHONINSTALL = pythoninstall
endif

# use distutils to build
pythonbuild: dev_blobfinder.c dev_fiducial.c dev_laser.c \
             dev_position.c dev_ptz.c dev_wifi.c pyplayerc.c pyplayerc.h setup.py
	@PYTHON_INTERP@ setup.py build


EXTRA_DIST = dev_blobfinder.c \
             dev_fiducial.c \
             dev_laser.c \
             dev_position.c \
             dev_ptz.c \
						 dev_wifi.c \
             pyplayerc.c \
             pyplayerc.h \
	     setup.py

clean-local:
	rm -rf build

install-data-hook: $(PYTHONINSTALL)

# use distutils to install
pythoninstall:
	@PYTHON_INTERP@ setup.py install --install-platlib=$(libdir)/python

# seems that distutils doesn't support 'uninstall', so we'll hack it manually
uninstall-local:
	rm -rf $(libdir)/python

