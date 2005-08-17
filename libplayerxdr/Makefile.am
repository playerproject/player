AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir)

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerxdr.pc

EXTRA_DIST = playerxdr.pc.in

BUILT_SOURCES = playerxdr.c playerxdr.h
CLEANFILES = playerxdr.pc playerxdr.c playerxdr.h
playerxdr.c: Makefile parse.py $(top_srcdir)/libplayercore/player.h
	python ./parse.py $(top_srcdir)/libplayercore/player.h playerxdr.c playerxdr.h

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LTLIBRARIES = libplayerxdr.la

libplayerxdr_la_SOURCES = playerxdr.c playerxdr.h functiontable.h functiontable.c

pkgincludedir = $(includedir)/player-@VERSION@/libplayerxdr
pkginclude_HEADERS = playerxdr.h functiontable.h
