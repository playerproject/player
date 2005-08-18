driverincludedir = $(includedir)/player-@VERSION@/drivers
driverlibdir = $(libdir)/player-@VERSION@

AM_CPPFLAGS = -Wall -I$(top_srcdir)

driverlib_LTLIBRARIES = 
if INCLUDE_LINUXJOYSTICK
driverlib_LTLIBRARIES += liblinuxjoystick.la
endif

liblinuxjoystick_la_SOURCES = linuxjoy.cc
