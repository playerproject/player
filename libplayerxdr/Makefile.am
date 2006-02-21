AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir) -fno-strict-aliasing

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgincludedir = $(includedir)/player-2.0/libplayerxdr

BUILT_SOURCES =
lib_LTLIBRARIES =
pkgconfig_DATA =
pkginclude_HEADERS = 
if HAVE_PYTHON
BUILT_SOURCES += playerxdr.c playerxdr.h
lib_LTLIBRARIES += libplayerxdr.la
pkgconfig_DATA += playerxdr.pc
pkginclude_HEADERS += playerxdr.h functiontable.h
endif

dist_bin_SCRIPTS = playerxdrgen.py

EXTRA_DIST = playerxdr.pc.in

CLEANFILES = playerxdr.pc playerxdr.c playerxdr.h

playerxdr.c: Makefile playerxdrgen.py $(top_srcdir)/libplayercore/player.h
	python $(srcdir)/playerxdrgen.py -distro $(top_srcdir)/libplayercore/player.h playerxdr.c playerxdr.h

libplayerxdr_la_SOURCES = playerxdr.c playerxdr.h functiontable.h functiontable.c
libplayerxdr_la_LDFLAGS = -version-info 2:0:0 -rpath $(libdir) $(top_builddir)/libplayercore/libplayererror.la

libplayerxdr_la_DEPENDENCIES = $(top_builddir)/libplayercore/libplayererror.la
