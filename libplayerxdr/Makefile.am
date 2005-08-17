AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir)

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkglibdir = $(libdir)/player-@VERSION@
pkgincludedir = $(includedir)/player-@VERSION@/libplayerxdr

BUILT_SOURCES =
pkglib_LTLIBRARIES =
pkgconfig_DATA =
pkginclude_HEADERS = 
if HAVE_PYTHON
BUILT_SOURCES += playerxdr.c playerxdr.h
pkglib_LTLIBRARIES += libplayerxdr.la
pkgconfig_DATA += playerxdr.pc
pkginclude_HEADERS += playerxdr.h functiontable.h
endif

EXTRA_DIST = playerxdr.pc.in

CLEANFILES = playerxdr.pc playerxdr.c playerxdr.h

playerxdr.c: Makefile parse.py $(top_srcdir)/libplayercore/player.h
	python ./parse.py $(top_srcdir)/libplayercore/player.h playerxdr.c playerxdr.h

libplayerxdr_la_SOURCES = playerxdr.c playerxdr.h functiontable.h functiontable.c

