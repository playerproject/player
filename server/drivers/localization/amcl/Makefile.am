AM_CPPFLAGS = -Wall -I$(top_srcdir)
if INCLUDE_RTKGUI
AM_CPPFLAGS += -I$(top_srcdir)/rtk2 @GTK_CFLAGS@
endif

noinst_LTLIBRARIES = 
if INCLUDE_AMCL
noinst_LTLIBRARIES += libamcl.la
endif


libamcl_la_SOURCES = amcl.h \
                     amcl.cc \
                     amcl_sensor.h \
                     amcl_sensor.cc \
                     amcl_odom.h \
                     amcl_odom.cc \
                     amcl_laser.h \
                     amcl_laser.cc \
                     models/odometry.h \
                     models/odometry.c \
                     models/sonar.h \
                     models/sonar.c \
                     models/laser.h \
                     models/laser.c \
                     models/wifi.h \
                     models/wifi.c \
                     models/gps.h \
                     models/gps.c \
                     models/imu.h \
                     models/imu.c \
                     pf/pf.c \
                     pf/pf_kdtree.c \
                     pf/pf_pdf.c \
                     pf/pf_vector.c \
                     pf/pf_draw.c \
                     pf/gsl_discrete.c \
                     pf/pf.h \
                     pf/pf_pdf.h \
                     pf/pf_kdtree.h \
                     pf/pf_vector.h \
                     map/map.c \
                     map/map_range.c \
                     map/map_store.c \
                     map/map_draw.c \
                     map/map.h

#amcl_gps.h amcl_gps.cc amcl_imu.h amcl_imu.cc amcl_sonar.cc amcl_wifi.cc
#amcl_fiducial.cc amcl_fiducial.h
