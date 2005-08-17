AM_CPPFLAGS = -g -fPIC -Wall -I$(top_srcdir)

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playertcp.pc

EXTRA_DIST = playertcp.pc.in
CLEANFILES = playertcp.pc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LTLIBRARIES = libplayertcp.la

libplayertcp_la_DEPENDENCIES = $(top_builddir)/libplayercore/libplayercore.la $(top_builddir)/libplayercore/libplayererror.la $(top_builddir)/libplayerxdr/libplayerxdr.la

libplayertcp_la_SOURCES = socket_util.c socket_util.h \
                         playertcp.cc playertcp.h

libplayertcp_la_LIBADD = $(top_builddir)/replace/*.lo

pkgincludedir = $(includedir)/player-@VERSION@/libplayertcp

pkginclude_HEADERS = playertcp.h
