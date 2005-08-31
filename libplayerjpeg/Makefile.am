AM_CPPFLAGS = -I$(top_srcdir) -Wall

noinst_LTLIBRARIES =
if INCLUDE_JPEG
noinst_LTLIBRARIES += libplayerjpeg.la
endif

libplayerjpeg_la_SOURCES = playerjpeg.h playerjpeg.c
libplayerjpeg_la_LIBADD = -ljpeg

