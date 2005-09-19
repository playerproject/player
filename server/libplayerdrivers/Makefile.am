#
# Here we do a little hack to copy all the driver object files into one
# directory and put them together into an archive lib.  The java
# libplayercore bindings also refer to *.o in this directory in order to
# suck in all the driver objects.
#

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerdrivers.pc
CLEANFILES = playerdrivers.pc

lib_LTLIBRARIES = libplayerdrivers.la

AM_CPPFLAGS = -I$(top_srcdir)

libplayerdrivers_la_SOURCES = driverregistry.cc driverregistry.h
libplayerdrivers_la_LDFLAGS = `find $(top_builddir)/server/drivers -name "*.lo"` -version-info 2:0:0 -rpath $(libdir)
libplayerdrivers_la_LIBADD = @PLAYER_DRIVER_EXTRA_LIBS@

pkgincludedir = $(includedir)/libplayerdrivers
pkginclude_HEADERS = driverregistry.h

all-local:
	touch driverregistry.cc
