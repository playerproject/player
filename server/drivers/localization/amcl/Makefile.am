SUBDIRS = pf map models

if INCLUDE_RTKGUI
AM_CPPFLAGS = -I$(top_srcdir)/server @RTK_CPPFLAGS@
else
AM_CPPFLAGS = -I$(top_srcdir)/server
endif

noinst_LIBRARIES = @AMCL_LIB@

EXTRA_LIBRARIES = libamcl.a

libamcl_a_SOURCES = amcl.cc
libamcl_a_LIBADD = pf/*.o map/*.o models/*.o
