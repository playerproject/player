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
pkglib_LIBRARIES = libplayerdrivers.a

libplayerdrivers_a_SOURCES = driverregistry.cc driverregistry.h

# TODO: move the driver code out of server/drivers
libplayerdrivers.a: driverregistry.o
	cp `find ../drivers -name "*.o"` .
	ar cr $@ *.o

