AM_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/c -I$(top_srcdir)/client_libs/c++

bin_PROGRAMS = playerjoy

playerjoy_SOURCES = playerjoy.cc
playerjoy_LDADD = -L$(top_builddir)/client_libs/c++ -lplayerclient
