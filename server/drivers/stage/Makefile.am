AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

noinst_LIBRARIES = @STAGE_LIB@ libpsdevice.a

EXTRA_LIBRARIES = libstage.a

libstage_a_SOURCES = stagedevice.cc stagedevice.h stage.h stagetime.cc \
                     stagetime.h

libpsdevice_a_SOURCES = psdevice.h psdevice.cc

include_HEADERS = stage.h
