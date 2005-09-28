noinst_LTLIBRARIES = 
if INCLUDE_WAVEFRONT
noinst_LTLIBRARIES += libwavefront.la
endif

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

libwavefront_la_SOURCES = plan.c plan_plan.c plan_waypoint.c plan.h \
                          wavefront.cc
