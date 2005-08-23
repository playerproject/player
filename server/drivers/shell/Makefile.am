driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

driverlib_LTLIBRARIES = 
if INCLUDE_LOGFILE
driverlib_LTLIBRARIES += liblogfile.la
endif

driverinclude_HEADERS = logfile.h

AM_CPPFLAGS = -g3 -Wall -I$(top_srcdir)

#libdummy_a_SOURCES = dummy.cc
#libpassthrough_a_SOURCES = passthrough.cc 
liblogfile_la_SOURCES = writelog.cc encode.h encode.cc

#readlog_time.h readlog_time.cc readlog.cc 
