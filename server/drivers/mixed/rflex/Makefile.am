driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

driverlib_LTLIBRARIES = 
if INCLUDE_RFLEX
driverlib_LTLIBRARIES += librflex.la
endif

driverinclude_HEADERS = rflex_commands.h rflex_configs.h rflex.h rflex-info.h rflex-io.h

AM_CPPFLAGS = -Wall -I$(top_srcdir)

librflex_la_SOURCES = rflex.cc rflex_commands.cc rflex-io.cc
