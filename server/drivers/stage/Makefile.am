
CXX = @CXX_SERVER@
AR = @AR_SERVER@
RANLIB = @RANLIB_SERVER@
CXXFLAGS = @CXXFLAGS_SERVER@


AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

noinst_LIBRARIES = libstage.a

libstage_a_SOURCES = stagedevice.cc stagedevice.h stage.h stagetime.cc \
                     stagetime.h

include_HEADERS = stage.h
