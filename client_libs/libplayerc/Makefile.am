SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/server

lib_LIBRARIES = libplayerc.a

libplayerc_a_SOURCES = playerc.h \
											 mclient.c \
											 client.c \
                       device.c \
											 utils.c \
                       dev_blobfinder.c \
                       dev_comms.c \
                       dev_fiducial.c \
                       dev_frf.c \
                       dev_gps.c \
                       dev_srf.c \
                       dev_position.c \
                       dev_ptz.c \
                       dev_truth.c \
                       error.c \
                       error.h

include_HEADERS = playerc.h

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/libplayerc.ps $(top_distdir)/doc
