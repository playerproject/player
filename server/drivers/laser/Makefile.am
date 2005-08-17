driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

driverlib_LTLIBRARIES = 
if INCLUDE_SICKLMS200
driverlib_LTLIBRARIES += libsicklms200.la
endif
if INCLUDE_LASERCSPACE
driverlib_LTLIBRARIES += liblasercspace.la
endif
if INCLUDE_SICKPLS
driverlib_LTLIBRARIES += libsickpls.la
endif

driverinclude_HEADERS = sicklms200.h lasercspace.h

AM_CPPFLAGS = -Wall -I$(top_srcdir)

libsicklms200_la_SOURCES = sicklms200.cc
liblasercspace_la_SOURCES = lasercspace.cc
libsickpls_la_SOURCES = sickpls.cc

