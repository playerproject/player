SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/replace

lib_LIBRARIES = libplayerc.a

libplayerc_a_SOURCES = playerc.h \
											 mclient.c \
											 client.c \
                       device.c \
											 utils.c \
                       dev_blobfinder.c \
                       dev_comms.c \
                       dev_fiducial.c \
                       dev_laser.c \
								       dev_localize.c \
                       dev_gps.c \
                       dev_sonar.c \
                       dev_bumper.c \
                       dev_position.c dev_position3d.c \
                       dev_power.c \
                       dev_ptz.c \
                       dev_truth.c \
                       dev_wifi.c \
                       error.c \
                       error.h

libplayerc_a_LIBADD = $(top_builddir)/replace/*.o

include_HEADERS = playerc.h

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make
	cp -p $(srcdir)/doc/libplayerc.ps $(top_distdir)/doc
