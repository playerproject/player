driverincludedir = $(includedir)/libplayerdrivers

AM_CPPFLAGS = -Wall -I$(top_srcdir) @GDK_PIXBUF_CFLAGS@

lib_LTLIBRARIES = 
if INCLUDE_MAPFILE
lib_LTLIBRARIES += libmapfile.la 
endif
if INCLUDE_MAPCSPACE
lib_LTLIBRARIES += libmapcspace.la
endif
if INCLUDE_MAPSCALE
lib_LTLIBRARIES += libmapscale.la
endif

libmapfile_la_SOURCES = mapfile.cc 
libmapfile_la_LDFLAGS = -version-info 2:0:0
libmapcspace_la_SOURCES = mapcspace.cc 
libmapcspace_la_LDFLAGS = -version-info 2:0:0
libmapscale_la_SOURCES = mapscale.cc 
libmapscale_la_LDFLAGS = -version-info 2:0:0
