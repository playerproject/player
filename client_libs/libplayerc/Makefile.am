SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/include

lib_LIBRARIES = libplayerc.a

libplayerc_a_SOURCES = client.c \
                       dev_bps.c \
                       dev_broadcast.c \
                       dev_gps.c \
                       device.c \
                       dev_laser.c \
                       dev_lbd.c \
                       dev_position.c \
                       dev_ptz.c \
                       dev_sonar.c \
                       dev_truth.c \
                       dev_vision.c \
                       error.c \
                       error.h \
                       mclient.c \
                       playerc.h

include_HEADERS = playerc.h

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/libplayerc.ps $(top_distdir)/doc
