noinst_LTLIBRARIES =

if INCLUDE_EEDHCONTROLLER
noinst_LTLIBRARIES += libeeDHcontroller.la
endif

AM_CPPFLAGS = -Wall -I$(top_srcdir) @EEDHCONTROLLER_EXTRA_CPPFLAGS@

libeeDHcontroller_la_SOURCES = eeDHcontroller.cc
libeeDHcontroller_la_LIBADD = @EEDHCONTROLLER_EXTRA_CPPFLAGS@
