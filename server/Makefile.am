SUBDIRS = drivers

AM_CPPFLAGS = -Wall -I$(srcdir)/drivers/stage

CXX = @CXX_SERVER@
AR = @AR_SERVER@
RANLIB = @RANLIB_SERVER@
CXXFLAGS = @CXXFLAGS_SERVER@


player_DEPENDENCIES = drivers/stage/libstage.a \
			@P2OS_LIBPATH@ \
		       	@SICK_LIBPATH@ \
		       	@FESTIVAL_LIBPATH@ \
			@ACTS_LIBPATH@ \
			@SONY_LIBPATH@ \
		 	@UDPBCAST_LIBPATH@ \
			@LASER_LIBPATH@ \
			@RWI_LIBPATH@ \
			@WIFI_LIBPATH@ \
			@FIXEDTONES_LIBPATH@ \
			@ISENSE_LIBPATH@ \
			@LIBOBJS@


#
# Note to everybody: please do NOT break the following assignment onto
# separate lines with backslashes.  It makes the link line look like shit and 
# hard to parse.  - BPG
#
player_LDADD   = -lpthread -lm $(top_builddir)/server/drivers/stage/libstage.a @REB_LIBPATH@ @P2OS_LIBPATH@ @SICK_LIBPATH@ @FESTIVAL_LIBPATH@ @ACTS_LIBPATH@ @SONY_LIBPATH@ @UDPBCAST_LIBPATH@ @LASER_LIBPATH@ @WIFI_LIBPATH@ @RWI_LIBPATH@ @FIXEDTONES_LIBPATH@ @FIXEDTONES_EXTRA_LIB@ @ISENSE_LIBPATH@ @ISENSE_EXTRA_LIB@ @LIBOBJS@

bin_PROGRAMS = player


lib_LIBRARIES = libplayerqueue.a 

libplayerqueue_a_SOURCES = playerqueue.cc

player_SOURCES = clientdata.cc socket_util.cc devicetable.cc device.cc \
                 deviceregistry.cc clientmanager.cc wallclocktime.cc main.cc \
                 playerqueue.cc configfile.cc playercommon.h \
                 player.h playerqueue.h device.h deviceregistry.h \
                 devicetable.h clientdata.h clientmanager.h socket_util.h \
                 wallclocktime.h playertime.h configfile.h drivertable.cc \
                 drivertable.h


include_HEADERS = playercommon.h \
                  player.h \
                  playerqueue.h

