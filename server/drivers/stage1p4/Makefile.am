AM_CPPFLAGS = -Wall -I$(top_srcdir)/server @STAGE1P4_CFLAGS@

#noinst_LIBRARIES = @STAGE_LIB@

noinst_LIBRARIES = @STAGE1P4_LIB@

EXTRA_LIBRARIES = libstage1p4.a

libstage1p4_a_SOURCES = \
	stage1p4.cc \
	stg_position.cc \
	stg_laser.cc \
	stg_simulation.cc \
	stg_fiducial_neighbors.cc


# This file should be added to CVS before it's referenced here.  Otherwise,
# the 'make dist' fails.
#	stage1p4.h 

#include_HEADERS = 
