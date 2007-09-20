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
EXTRA_DIST = Makefile.example example.cfg eginterf_client.h eginterf.h eginterf_xdr.h

eginterf_xdr.c: eginterf.h ${top_builddir}/libplayerxdr/playerxdrgen.py
	python ${top_builddir}/libplayerxdr/playerxdrgen.py eginterf.h eginterf_xdr.c eginterf_xdr.h

eginterf_xdr.h: eginterf.h ${top_builddir}/libplayerxdr/playerxdrgen.py
	python ${top_builddir}/libplayerxdr/playerxdrgen.py eginterf.h eginterf_xdr.c eginterf_xdr.h

example_LTLIBRARIES = libeginterfdriver.la libeginterf.la
libeginterfdriver_la_SOURCES = eginterf_driver.cc
libeginterf_la_SOURCES = eginterf_client.c eginterf.c eginterf_xdr.c

example_client_SOURCES = example_client.c
example_client_LDADD = -lplayerc $(PLAYERC_LIB_DIRS) -lplayerxdr -lplayererror libeginterf.la
example_client_DEPENDENCIES = $(PLAYERC_DEPS)
