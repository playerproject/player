PLAYERC_CFLAGS = -I$(top_srcdir) \
                 -I$(top_srcdir)/client_libs/

PLAYERC_LIB_DIRS=-L${top_builddir}/libplayerxdr \
                 -L${top_builddir}/client_libs/libplayerc

PLAYERC_DEPS = $(top_builddir)/client_libs/libplayerc/libplayerc.la \
               $(top_builddir)/libplayerxdr/libplayerxdr.la

AM_CPPFLAGS = $(PLAYERC_CFLAGS) -Wall -g

noinst_PROGRAMS = simple

simple_SOURCES = simple.c
simple_LDADD = $(PLAYERC_LIB_DIRS) -lm -lplayerxdr -lplayerc
simple_DEPENDENCIES = $(PLAYERC_DEPS)
