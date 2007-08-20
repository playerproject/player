AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir) -fno-strict-aliasing

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgincludedir = $(includedir)/player-2.0/libplayerxdr

BUILT_SOURCES =
if HAVE_PYTHON
BUILT_SOURCES += playerxdr.c playerxdr.h functiontable_gen.c
endif

lib_LTLIBRARIES = libplayerxdr.la
pkgconfig_DATA = playerxdr.pc
pkginclude_HEADERS = playerxdr.h functiontable.h

dist_bin_SCRIPTS = playerxdrgen.py

EXTRA_DIST = playerxdr.pc.in playerxdr.html playerxdr.c playerxdr.h

CLEANFILES = playerxdr.pc

functiontable_gen.c: $(top_srcdir)/libplayercore/playerinterfacegen.py $(top_srcdir)/libplayercore/interfaces/*
	if ! $(top_srcdir)/libplayercore/playerinterfacegen.py --functiontable $(top_srcdir)/libplayercore/interfaces/ > functiontable_gen.c; then rm functiontable_gen.c; fi

$(top_srcdir)/libplayercore/player_interfaces.h: $(top_srcdir)/libplayercore/interfaces/*
	make -C $(top_srcdir)/libplayercore/ player_interfaces.h

playerxdr.h: playerxdr.c

playerxdr.c: playerxdrgen.py $(top_srcdir)/libplayercore/player.h $(top_srcdir)/libplayercore/player_interfaces.h
	python $(srcdir)/playerxdrgen.py -distro $(top_srcdir)/libplayercore/player.h playerxdr.c playerxdr.h $(top_srcdir)/libplayercore/player_interfaces.h

libplayerxdr_la_SOURCES = playerxdr.c playerxdr.h functiontable.h functiontable.c
# Can't use libplayererror here because of an unresolved circular build
# dependency
#libplayerxdr_la_LDFLAGS = $(PLAYER_VERSION_INFO) -rpath $(libdir) $(top_builddir)/libplayercore/libplayererror.la
libplayerxdr_la_LDFLAGS = $(PLAYER_VERSION_INFO) -rpath $(libdir)

# Can't use libplayererror here because of an unresolved circular build
# dependency
#libplayerxdr_la_DEPENDENCIES = $(top_builddir)/libplayercore/libplayererror.la
