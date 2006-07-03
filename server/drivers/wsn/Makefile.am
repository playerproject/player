noinst_LTLIBRARIES = 

if INCLUDE_MICA2
noinst_LTLIBRARIES += libmica2.la
endif

if INCLUDE_RCORE_XBRIDGE
noinst_LTLIBRARIES += librcore_xbridge.la
endif

#if INCLUDE_ACCEL_CALIB
#noinst_LTLIBRARIES += libaccel_calib.la
#endif

AM_CPPFLAGS = -Wall -I$(top_srcdir) -fno-strict-aliasing

libmica2_la_SOURCES = mica2.cc
librcore_xbridge_la_SOURCES = rcore_xbridge.cc
#libaccel_calib_la_SOURCES = accel_calib.cc
