SUBDIRS = pf map models

if INCLUDE_RTKGUI
AM_CPPFLAGS = -Wall -I$(top_srcdir)/server @RTK_CPPFLAGS@
else
AM_CPPFLAGS = -Wall -I$(top_srcdir)/server
endif

noinst_LIBRARIES = @AMCL_LIB@

EXTRA_LIBRARIES = libamcl.a

libamcl_a_SOURCES = amcl.h amcl.cc amcl_odom.cc amcl_sonar.cc amcl_laser.cc amcl_wifi.cc amcl_gps.cc amcl_imu.cc
libamcl_a_LIBADD = pf/*.o map/*.o models/*.o
