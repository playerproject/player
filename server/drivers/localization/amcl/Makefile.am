SUBDIRS = pf map models

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = @AMCL_LIB@

EXTRA_LIBRARIES = libamcl.a

libamcl_a_SOURCES = amcl.cc
libamcl_a_LIBADD = pf/*.o map/*.o models/*.o
