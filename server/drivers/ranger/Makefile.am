noinst_LTLIBRARIES =

if INCLUDE_LASERTORANGER
noinst_LTLIBRARIES += liblasertoranger.la
endif

if INCLUDE_SONARTORANGER
noinst_LTLIBRARIES += libsonartoranger.la
endif


AM_CPPFLAGS = -Wall -I$(top_srcdir)

liblasertoranger_la_SOURCES = toranger.cc lasertoranger.cc

libsonartoranger_la_SOURCES = toranger.cc sonartoranger.cc