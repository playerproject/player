if WITH_STAGE
BUILDSTAGE = libstage.a
else
BUILDSTAGE = 
endif

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = $(BUILDSTAGE)

libstage_a_SOURCES = stagedevice.cc stagedevice.h stage.h stagetime.cc \
                     stagetime.h

include_HEADERS = stage.h

clean-local:
	rm -f *.a
