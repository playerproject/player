SUBDIRS = drivers

AM_CPPFLAGS = -I$(srcdir)/drivers/stage

bin_PROGRAMS = player

lib_LIBRARIES = libplayerqueue.a

libplayerqueue_a_SOURCES = playerqueue.cc

player_SOURCES = clientdata.cc socket_util.cc devicetable.cc device.cc \
              deviceregistry.cc clientmanager.cc wallclocktime.cc main.cc \
              playerqueue.cc configfile.cc defaults.h playercommon.h \
	      player.h messages.h playerqueue.h device.h deviceregistry.h \
	      devicetable.h clientdata.h clientmanager.h socket_util.h \
	      wallclocktime.h playertime.h configfile.h

player_DEPENDENCIES = @SICKLMS200_LIB@ @FESTIVAL_LIB@ @ACTS_LIB@ @SONYEVID30_LIB@ @P2OS_LIB@ @UDPBROADCAST_LIB@ @LASERBARCODE_LIB@ @STAGE_LIB@ @RWI_LIB@

player_LDADD   = -lpthread -lm -ldl @SICKLMS200_LIB@ @FESTIVAL_LIB@ @ACTS_LIB@ @SONYEVID30_LIB@ @P2OS_LIB@ @UDPBROADCAST_LIB@ @LASERBARCODE_LIB@ @STAGE_LIB@ @RWI_LIB@

include_HEADERS = defaults.h \
                  playercommon.h \
                  player.h \
                  messages.h \
                  playerqueue.h

