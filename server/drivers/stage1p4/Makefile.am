AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace @STAGE1P4_CFLAGS@

noinst_LIBRARIES = @STAGE1P4_LIB@

EXTRA_LIBRARIES = libstage1p4.a

libstage1p4_a_SOURCES = \
 stage1p4.cc \
 stage1p4.h \
 stg_time.cc \
 stg_time.h \
 stg_simulation.cc \
 stg_laser.cc \
 stg_position.cc \
 stg_sonar.cc \
 stg_energy.cc \
 stg_blobfinder.cc \
 stg_fiducial.cc 

#	stg_blinkenlight.cc
# worldfile.cc \
# worldfile.h 

# This file should be added to CVS before it's referenced here.  Otherwise,
# the 'make dist' fails.
#	stage1p4.h 

#include_HEADERS = 
