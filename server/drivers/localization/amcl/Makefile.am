SUBDIRS = pf map models

if INCLUDE_RTKGUI
AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/server/drivers/stage -I@RTK_DIR@/include `gtk-config --cflags`
else
AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/server/drivers/stage
endif

noinst_LIBRARIES = @AMCL_LIB@

EXTRA_LIBRARIES = libamcl.a

libamcl_a_SOURCES = amcl.cc
libamcl_a_LIBADD = pf/*.o map/*.o models/*.o
