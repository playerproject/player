AM_CPPFLAGS = -I$(top_srcdir) -Wall
lib_LTLIBRARIES =

noinst_LTLIBRARIES =
if INCLUDE_JPEG
lib_LTLIBRARIES += libplayerjpeg.la
endif

libplayerjpeg_la_SOURCES = playerjpeg.h playerjpeg.c
libplayerjpeg_la_LDFLAGS = -version-info 2:0:0 -rpath $(libdir)
libplayerjpeg_la_LIBADD = -ljpeg

