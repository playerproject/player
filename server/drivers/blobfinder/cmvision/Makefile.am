#AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/libplayerpacket -DUSE_METEOR

AM_CPPFLAGS = -Wall -I$(top_srcdir) -DUSE_METEOR

noinst_LTLIBRARIES =
if INCLUDE_CMVISION
noinst_LTLIBRARIES += libcmvision.la
endif

libcmvision_la_SOURCES = cmvision.cc cmvision.h conversions.c conversions.h capture.h P2CMV.cc
