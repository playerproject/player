noinst_LTLIBRARIES =
if INCLUDE_POSTGIS
noinst_LTLIBRARIES += libpostgis.la
endif

AM_CPPFLAGS = -Wall -I$(top_srcdir) $(LIBPQXX_CFLAGS)

libpostgis_la_SOURCES = postgis.cc dbconn.cc dbconn.h
libpostgis_la_LIBADD = @GEOS_LIBS@