driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

AM_CPPFLAGS = -Wall -I$(top_srcdir) @GDK_PIXBUF_CFLAGS@

driverlib_LTLIBRARIES = 
if INCLUDE_MAPFILE
driverlib_LTLIBRARIES += libmapfile.la 
endif
if INCLUDE_MAPCSPACE
driverlib_LTLIBRARIES += libmapcspace.la
endif
if INCLUDE_MAPSCALE
driverlib_LTLIBRARIES += libmapscale.la
endif

libmapfile_la_SOURCES = mapfile.cc 
libmapcspace_la_SOURCES = mapcspace.cc 
libmapscale_la_SOURCES = mapscale.cc 
