driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

noinst_LIBRARIES = @LASERCSPACE_LIB@ @SICKPLS_LIB@
driverlib_LIBRARIES = @SICKLMS200_LIB@
driverinclude_HEADERS = sicklms200.h

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace

EXTRA_LIBRARIES = libsicklms200.a liblasercspace.a libsickpls.a

libsicklms200_a_SOURCES = sicklms200.cc
liblasercspace_a_SOURCES = lasercspace.cc
libsickpls_a_SOURCES = sickpls.cc

