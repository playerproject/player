AM_CPPFLAGS = -I$(top_srcdir)/server

CC = @CC_CLIENTS@
CXX = @CXX_CLIENTS@
AR = @AR_CLIENTS@
RANLIB = @RANLIB_CLIENTS@
CXXFLAGS = @CXXFLAGS_CLIENTS@
CFLAGS = @CFLAGS_CLIENTS@

lib_LIBRARIES = libplayercclient.a

libplayercclient_a_SOURCES = helpers.c playercclient.c playercclient.h print.c

include_HEADERS = playercclient.h

