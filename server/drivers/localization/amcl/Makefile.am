SUBDIRS = pf map models

if INCLUDE_RTKGUI
AM_CPPFLAGS = -Wall -I$(top_srcdir)/server @GSL_CFLAGS@ -I$(top_srcdir)/rtk2 @GTK_CFLAGS@
else
AM_CPPFLAGS = -Wall -I$(top_srcdir)/server @GSL_CFLAGS@
endif

noinst_LIBRARIES = @AMCL_LIB@

EXTRA_LIBRARIES = libamcl.a

libamcl_a_SOURCES = amcl.h amcl.cc amcl_sensor.h amcl_sensor.cc amcl_odom.h amcl_odom.cc amcl_laser.h amcl_laser.cc amcl_fiducial.cc amcl_fiducial.h #amcl_gps.h amcl_gps.cc amcl_imu.h amcl_imu.cc amcl_sonar.cc amcl_wifi.cc
libamcl_a_LIBADD = pf/*.o map/*.o models/*.o
