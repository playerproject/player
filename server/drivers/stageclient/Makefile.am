AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace @STAGECLIENT_CFLAGS@

noinst_LIBRARIES = @STAGECLIENT_LIB@

EXTRA_LIBRARIES = libstageclient.a

libstageclient_a_SOURCES = \
 stageclient.cc \
 stageclient.h \
 stg_time.cc \
 stg_time.h \
 stg_simulation.cc \
 stg_laser.cc \
 stg_position.cc \
 stg_sonar.cc \
 stg_fiducial.cc \
 stg_blobfinder.cc 

 #stg_energy.cc 
