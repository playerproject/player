AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/libplayerc
LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm

CC = @CC_CLIENTS@
CXX = @CXX_CLIENTS@
AR = @AR_CLIENTS@
RANLIB = @RANLIB_CLIENTS@
CXXFLAGS = @CXXFLAGS_CLIENTS@
CFLAGS = @CFLAGS_CLIENTS@

noinst_PROGRAMS = simple

simple_SOURCES = simple.c

