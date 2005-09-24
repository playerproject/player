PLAYERCC_CFLAGS = -I$(srcdir)/client_libs/libplayerc++ -I$(top_srcdir) -I$(top_srcdir)/client_libs/libplayerc
PLAYERCC_LIBS = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc  -L$(top_builddir)/client_libs/libplayerc++ -lplayerc++ -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lm

AM_CPPFLAGS = $(PLAYERCC_CFLAGS) -Wall -g
# this should be cleaned, but no time now
example1_LDADD = $(PLAYERCC_LIBS)
example2_LDADD = $(PLAYERCC_LIBS)

bin_PROGRAMS = example1 example2

example1_SOURCES = example1.cc

example2_SOURCES = example2.cc
