#
# Here we do a little hack to copy all the driver object files into one
# directory and put them together into an archive lib.  The java
# libplayercore bindings also refer to *.o in this directory in order to
# suck in all the driver objects.
#

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerdrivers.pc
CLEANFILES = playerdrivers.pc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LTLIBRARIES = libplayerdrivers.la

AM_CPPFLAGS = -I$(top_srcdir)

libplayerdrivers_la_SOURCES = driverregistry.cc driverregistry.h
libplayerdrivers_la_LDFLAGS = `find $(top_builddir)/server/drivers -name "*.lo"`
#libplayerdrivers_la_LIBADD = $(top_builddir)/libplayercore/libplayercore.la $(top_builddir)/libplayercore/libplayererror.la

# TODO: move the driver code out of server/drivers
#libplayerdrivers.la: driverregistry.lo
#	cp `find ../drivers -name "*.lo"` .
#	ar cr $@ *.lo

