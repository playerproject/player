AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/replace

# create the pkg-config entry
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playercclient.pc

lib_LIBRARIES = libplayercclient.a
#lib_LTLIBRARIES = libplayercclient.la

libplayercclient_a_SOURCES = playercclient.c playercclient.h
#libplayercclient_la_SOURCES = playercclient.c playercclient.h
#libplayercclient_la_LDFLAGS = -version-info 1:0:0

include_HEADERS = playercclient.h
