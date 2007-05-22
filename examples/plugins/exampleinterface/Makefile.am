PLAYERC_CFLAGS = -I$(top_srcdir) \
                 -I$(top_srcdir)/client_libs/

PLAYERC_LIB_DIRS=-L${top_builddir}/libplayerxdr \
                 -L${top_builddir}/libplayercore \
                 -L${top_builddir}/client_libs/libplayerc

PLAYERC_DEPS = $(top_builddir)/client_libs/libplayerc/libplayerc.la \
               $(top_builddir)/libplayerxdr/libplayerxdr.la \
               $(top_builddir)/libplayercore/libplayererror.la

AM_CPPFLAGS = -Wall -g $(PLAYERC_CFLAGS)

exampledir = $(prefix)/share/player/examples/plugins/exampleinterface

example_PROGRAMS = example_client
example_DATA = eginterf_client.c eginterf.c eginterf_xdr.c eginterf_driver.cc Makefile.example example.cfg
EXTRA_DIST = Makefile.example example.cfg

example_LTLIBRARIES = libeginterfdriver.la
libeginterfdriver_la_SOURCES = eginterf_driver.cc
libeginterf_la_SOURCEs = eginterf_client.c eginterf.c eginterf_xdr.c

example_client_SOURCES = example_client.c
example_client_LDADD = -lplayerc $(PLAYERC_LIB_DIRS) -lplayerxdr -lplayererror
example_client_DEPENDENCIES = $(PLAYERC_DEPS)
