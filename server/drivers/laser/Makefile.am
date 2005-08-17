driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

driverlib_LTLIBRARIES = @SICKLMS200_LIB@ @LASERCSPACE_LIB@ @SICKPLS_LIB@
driverinclude_HEADERS = sicklms200.h lasercspace.h

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace

EXTRA_LTLIBRARIES = libsicklms200.la liblasercspace.la libsickpls.la

libsicklms200_la_SOURCES = sicklms200.cc
libsicklms200_la_LDFLAGS = -rpath '$(libdir)'
liblasercspace_la_SOURCES = lasercspace.cc
liblasercspace_la_LDFLAGS = -rpath '$(libdir)'
libsickpls_la_SOURCES = sickpls.cc

