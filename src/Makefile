#
# $Id$
#

#
#
# DO NOT make configuration changes here!  make them in the top-level
# Makefile.common.
#
 
include ../Makefile.common

SERVERNAME = player
DEVICELIBNAME = libplayerdevice.a

INCLUDES = -I../include

CFLAGS = -g -Wall $(PLAYER_PLATFORM)
LIBDIRS = -L.
LIBS = $(LIBDIRS) -lplayerdevice -lpthread -lm

INCLUDED_DEVICES = $(INCLUDE_LASER) $(INCLUDE_SONAR) $(INCLUDE_VISION) \
                   $(INCLUDE_POSITION) $(INCLUDE_GRIPPER) $(INCLUDE_MISC) \
                   $(INCLUDE_MISC) $(INCLUDE_PTZ) $(INCLUDE_STAGE) \
                   $(INCLUDE_AUDIO) $(INCLUDE_LASERBEACON) \
                   $(INCLUDE_BROADCAST) $(INCLUDE_SPEECH)

#
# definitely build the device-independent objects
#
OBS = nodevice.o clientdata.o lock.o pubsub_util.o devicetable.o 

#
# only build the device-specific objects that we need (duplicate objects
# won't cause problems)
#
ifdef INCLUDE_LASER
  OBS += laserdevice.o 
endif
ifdef INCLUDE_SONAR
  OBS += sonardevice.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_VISION
  OBS += visiondevice.o 
endif
ifdef INCLUDE_POSITION
  OBS += positiondevice.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_GRIPPER
  OBS += gripperdevice.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_MISC
  OBS += miscdevice.o p2osdevice.o packet.o sip.o 
endif
ifdef INCLUDE_PTZ
  OBS += ptzdevice.o 
endif
ifdef INCLUDE_STAGE
  OBS += arenalock.o stagedevice.o 
endif
ifdef INCLUDE_AUDIO
  OBS += audiodevice.o
  EXTRA_LIBS = -lrfftw -lfftw
endif
ifdef INCLUDE_LASERBEACON
  OBS += laserdevice.o laserbeacondevice.o
endif
ifdef INCLUDE_BROADCAST
  OBS += broadcastdevice.o 
endif
ifdef INCLUDE_SPEECH
  OBS += speechdevice.o
endif

all: $(SERVERNAME)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean:
	rm -f *.o *~ $(SERVERNAME) $(DEVICELIBNAME) core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): main.cc $(DEVICELIBNAME) 
	$(CPP) $(CFLAGS) -DPLAYER_VERSION=\"$(PLAYER_VERSION)\" $(INCLUDES) $(INCLUDED_DEVICES) -o $@ main.cc $(LIBS) $(EXTRA_LIBS) $(PLATFORM_LIBS)

%.o: %.cc 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<


install: $(SERVERNAME)
	$(MKDIR) -p $(INSTALL_BIN)
	$(INSTALL) -m 755 $(SERVERNAME) $(INSTALL_BIN)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 ../include/playercommon.h $(INSTALL_INCLUDE)/playercommon.h
	$(INSTALL) -m 644 ../include/messages.h $(INSTALL_INCLUDE)/messages.h
	$(INSTALL) -m 644 ../include/stage.h $(INSTALL_INCLUDE)/stage.h

uninstall:
	$(RM) -f $(INSTALL_BIN)/$(SERVERNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playercommon.h
	$(RM) -f $(INSTALL_INCLUDE)/messages.h
	$(RM) -f $(INSTALL_INCLUDE)/stage.h

