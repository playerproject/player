SUBDIRS = drivers

AM_CPPFLAGS = -I$(srcdir)/drivers/stage

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

player_DEPENDENCIES = drivers/p2os/libp2os.a drivers/stage/libstage.a drivers/libsicklms200.a drivers/libfestival.a drivers/libacts.a drivers/libsonyevid30.a drivers/libudpbroadcast.a drivers/liblaserbarcode.a @RWI_LIBPATH@ @FIXEDTONES_LIBPATH@

player_LDADD   = -lpthread -lm -ldl drivers/p2os/libp2os.a drivers/stage/libstage.a drivers/libsicklms200.a drivers/libfestival.a drivers/libacts.a drivers/libsonyevid30.a drivers/libudpbroadcast.a drivers/liblaserbarcode.a @RWI_LIBPATH@ @FIXEDTONES_LIBPATH@ @FIXEDTONES_EXTRA_LIB@

include_HEADERS = playercommon.h \
                  player.h \
                  playerqueue.h

