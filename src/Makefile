#
# $Id$
#

#
#
# DO NOT make configuration changes here!  make them in the top-level
# Makefile.common.
#
 
include ../Makefile.common

PWD=$(shell pwd)

SERVERNAME = player
DEVICELIBNAME = libplayerdevice.a

INCLUDES = -I../include

LIBDIRS = -L.
LIBS = $(LIBDIRS) -lplayerdevice -lpthread -lm -ldl

#
# definitely build the device-independent objects
#
OBS = clientdata.o pubsub_util.o devicetable.o device.o deviceregistry.o\
      clientmanager.o wallclocktime.o main.o playerqueue.o

#
# only build the device-specific objects that we need (duplicate objects
# won't cause problems)
#
ifdef INCLUDE_LASER
  OBS += laserdevice.o 
endif
ifdef INCLUDE_SONAR
  OBS += sonardevice.o p2osdevice.o robot_params.o packet.o sip.o 
endif
ifdef INCLUDE_VISION
  OBS += visiondevice.o 
endif
ifdef INCLUDE_POSITION
  OBS += positiondevice.o robot_params.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_GRIPPER
  OBS += gripperdevice.o robot_params.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_MISC
  OBS += miscdevice.o robot_params.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_PTZ
  OBS += ptzdevice.o 
endif
ifdef INCLUDE_STAGE
  OBS += stagedevice.o stagetime.o
endif
ifdef INCLUDE_AUDIO
  OBS += audiodevice.o
  EXTRA_LIBS += -lrfftw -lfftw
endif
ifdef INCLUDE_LASERBEACON
  OBS += laserdevice.o laserbeacondevice.o
endif
ifdef INCLUDE_BPS
  OBS += laserdevice.o laserbeacondevice.o bpsdevice.o
endif
ifdef INCLUDE_BROADCAST
  OBS += broadcastdevice.o 
endif
ifdef INCLUDE_SPEECH
  OBS += speechdevice.o
endif

#SHARED_OBJS = laserdevice.so

# we build the server, any dynamically loadable device drivers, and an archive
# of the PlayerQueue for linking into Stage
all: $(SERVERNAME) $(SHARED_OBJS) libplayerqueue.a

libplayerqueue.a: playerqueue.o
	$(AR) cr $@ $<
	$(MKDIR) -p ../lib
	$(LN) -fs $(PWD)/$@ ../lib

%.so: %.o 
	$(CPP) -shared -nostartfiles -o $@ $<

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak
clean:
	rm -f *.so *.o *~ $(SERVERNAME) $(DEVICELIBNAME) libplayerqueue.a \
	                  ../lib/libplayerqueue.a core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): $(DEVICELIBNAME) 
	$(CPP) -rdynamic -o $@ $(LIBS) $(EXTRA_LIBS) $(PLATFORM_LIBS)

main.o: main.cc
	$(CPP) $(CFLAGS) -DPLAYER_VERSION=\"$(PLAYER_VERSION)\" $(INCLUDES) $(INCLUDE_STAGE) -c $<

deviceregistry.o: deviceregistry.cc
	$(CPP) $(CFLAGS) $(INCLUDES) $(INCLUDED_DEVICES) -c $<

%.o: %.cc 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<


install: all
	$(MKDIR) -p $(INSTALL_BIN)
	$(INSTALL) -m 755 $(SERVERNAME) $(INSTALL_BIN)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 ../include/player.h $(INSTALL_INCLUDE)/player.h
	$(INSTALL) -m 644 ../include/playercommon.h $(INSTALL_INCLUDE)/playercommon.h
	$(INSTALL) -m 644 ../include/messages.h $(INSTALL_INCLUDE)/messages.h
	$(INSTALL) -m 644 ../include/defaults.h $(INSTALL_INCLUDE)/defaults.h
	$(INSTALL) -m 644 ../include/playerqueue.h $(INSTALL_INCLUDE)/playerqueue.h
	$(INSTALL) -m 644 ../include/stage.h $(INSTALL_INCLUDE)/stage.h
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 libplayerqueue.a $(INSTALL_LIB)/libplayerqueue.a

uninstall:
	$(RM) -f $(INSTALL_BIN)/$(SERVERNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playercommon.h
	$(RM) -f $(INSTALL_INCLUDE)/messages.h
	$(RM) -f $(INSTALL_INCLUDE)/defaults.h
	$(RM) -f $(INSTALL_INCLUDE)/stage.h
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
