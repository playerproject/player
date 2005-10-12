PLAYERCC_CFLAGS = -I$(top_srcdir) \
                  -I$(top_srcdir)/client_libs/ \
                  $(PLAYERCC_EXTRA_CFLAGS)

PLAYERCC_LIBS = -L$(top_builddir)/libplayerxdr \
                -L$(top_builddir)/client_libs/libplayerc \
                -L$(top_builddir)/client_libs/libplayerc++ \
                $(PLAYERCC_LIB)

PLAYERCC_DEPS = $(top_builddir)/client_libs/libplayerc/libplayerc.la \
                $(top_builddir)/libplayerxdr/libplayerxdr.la \
                $(top_builddir)/client_libs/libplayerc++/libplayerc++.la

AM_CPPFLAGS = $(PLAYERCC_CFLAGS) -Wall -g

bin_PROGRAMS =
if INCLUDE_PLAYERCC
  bin_PROGRAMS += example0 example1 example2 example3
endif

example0_SOURCES = example0.cc
example0_LDADD = $(PLAYERCC_LIBS)
example0_DEPENDENCIES = $(PLAYERCC_DEPS)

example1_SOURCES = example1.cc
example1_LDADD = $(PLAYERCC_LIBS)
example1_DEPENDENCIES = $(PLAYERCC_DEPS)

example2_SOURCES = example2.cc
example2_LDADD = $(PLAYERCC_LIBS)
example2_DEPENDENCIES = $(PLAYERCC_DEPS)

example3_SOURCES = example3.cc
example3_LDADD = $(PLAYERCC_LIBS)
example3_DEPENDENCIES = $(PLAYERCC_DEPS)
