driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

driverlib_LTLIBRARIES = 
if INCLUDE_BUMPERSAFE
driverlib_LTLIBRARIES += libbumpersafe.la
endif

driverinclude_HEADERS = 

AM_CPPFLAGS = -Wall -I$(top_srcdir)

libbumpersafe_la_SOURCES = bumpersafe.cc




