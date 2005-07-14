AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir)

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerxdr.pc

EXTRA_DIST = playerxdr.pc.in

BUILT_SOURCES = pack.c pack.h
CLEANFILES = playerxdr.pc pack.c pack.h
pack.c: Makefile parse.py
	python ./parse.py $(top_srcdir)/libplayercore/player.h pack.c pack.h

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LIBRARIES = libplayerxdr.a

libplayerxdr_a_SOURCES = pack.c pack.h

pkgincludedir = $(includedir)/player-@VERSION@/libplayerxdr
pkginclude_HEADERS = pack.h
