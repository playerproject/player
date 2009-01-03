noinst_LTLIBRARIES =

if INCLUDE_LASERTORANGER
noinst_LTLIBRARIES += liblasertoranger.la
endif

if INCLUDE_SONARTORANGER
noinst_LTLIBRARIES += libsonartoranger.la
endif

if INCLUDE_GBXSICKACFR
noinst_LTLIBRARIES += libgbxsickacfr.la
endif

if INCLUDE_HOKUYO_AIST
noinst_LTLIBRARIES += libhokuyo_aist.la
endif

AM_CPPFLAGS = -Wall -I$(top_srcdir)

liblasertoranger_la_SOURCES = toranger.cc lasertoranger.cc toranger.h

libsonartoranger_la_SOURCES = toranger.cc sonartoranger.cc toranger.h

libgbxsickacfr_la_SOURCES = gbxsickacfr.cc
libgbxsickacfr_la_CXXFLAGS = @GBXSICKACFR_CFLAGS@
libgbxsickacfr_la_LIBADD = @GBXSICKACFR_LIBS@

libhokuyo_aist_la_SOURCES = hokuyo_aist.cc
libhokuyo_aist_la_CXXFLAGS = @HOKUYO_AIST_CFLAGS@
libhokuyo_aist_la_LIBADD = @HOKUYO_AIST_LIBS@

