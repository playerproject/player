AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/c -I$(top_srcdir)/client_libs/c++
LDADD = -L$(top_builddir)/client_libs/c++ -lplayerclient

CC = @CC_CLIENTS@
CXX = @CXX_CLIENTS@
AR = @AR_CLIENTS@
RANLIB = @RANLIB_CLIENTS@
CXXFLAGS = @CXXFLAGS_CLIENTS@
CFLAGS = @CFLAGS_CLIENTS@

bin_PROGRAMS = playerjoy

playerjoy_SOURCES = playerjoy.cc
