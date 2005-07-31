AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir)

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playertcp.pc

EXTRA_DIST = playertcp.pc.in
CLEANFILES = playertcp.pc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LIBRARIES = libplayertcp.a

libplayertcp_a_DEPENDENCIES = $(top_builddir)/libplayercore/libplayercore.a $(top_builddir)/libplayercore/libplayererror.a $(top_builddir)/libplayerxdr/libplayerxdr.a

libplayertcp_a_SOURCES = socket_util.c socket_util.h \
                         playertcp.cc playertcp.h

libplayertcp_a_LIBADD = $(top_builddir)/replace/*.$(OBJEXT)

pkgincludedir = $(includedir)/player-@VERSION@/libplayertcp

pkginclude_HEADERS = playertcp.h
