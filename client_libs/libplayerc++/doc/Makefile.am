AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir) -I$(top_srcdir)/client_libs/libplayerc -Wall -g
# this should be cleaned, but no time now
example1_LDADD = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc  -L$(top_builddir)/client_libs/libplayerc++ -lplayerc++ -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lm
example2_LDADD = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc  -L$(top_builddir)/client_libs/libplayerc++ -lplayerc++ -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lm

bin_PROGRAMS = example1 example2

example1_SOURCES = example1.cc

example2_SOURCES = example2.cc
