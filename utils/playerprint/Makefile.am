AM_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/libplayerc++ -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/client_libs
LDADD = $(top_builddir)/client_libs/libplayerc++/.libs/libplayerc++.a -l@BOOST_THREAD_LIB@ -l@BOOST_SIGNALS_LIB@ $(top_builddir)/client_libs/libplayerc/.libs/libplayerc.a $(top_builddir)/libplayerxdr/.libs/libplayerxdr.a $(top_builddir)/libplayercore/.libs/libplayercore.a 

#bin_PROGRAMS = 
#if INCLUDE_BOOST_SIGNALS
#  bin_PROGRAMS += playerprint
#endif
#
#playerprint_SOURCES = playerprint.cc
