AM_CPPFLAGS = -Wall -I$(top_srcdir) @GDK_PIXBUF_CFLAGS@

noinst_LTLIBRARIES = 
if INCLUDE_MAPFILE
noinst_LTLIBRARIES += libmapfile.la 
endif
if INCLUDE_MAPCSPACE
noinst_LTLIBRARIES += libmapcspace.la
endif
if INCLUDE_MAPSCALE
noinst_LTLIBRARIES += libmapscale.la
endif

libmapfile_la_SOURCES = mapfile.cc 
libmapcspace_la_SOURCES = mapcspace.cc 
libmapscale_la_SOURCES = mapscale.cc 
