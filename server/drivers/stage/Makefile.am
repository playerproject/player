AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

noinst_LIBRARIES = @STAGE_LIB@

EXTRA_LIBRARIES = libstage.a

libstage_a_SOURCES = stagedevice.cc stagedevice.h stage1p3.h stagetime.cc \
                     stagetime.h

# this messes up the Stage 1.3 installation.
include_HEADERS = stage1p3.h
