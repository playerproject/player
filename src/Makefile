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

#CFLAGS = -g -Wall $(PLAYER_PLATFORM)
CFLAGS = -g3 -Wall $(PLAYER_PLATFORM)
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

# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

arenalock.o: ../include/device.h ../include/lock.h ../include/playercommon.h
arenalock.o: ../include/arenalock.h ../include/errors.h
audiodevice.o: ../include/audiodevice.h ../include/lock.h ../include/device.h
audiodevice.o: ../include/playercommon.h
broadcastdevice.o: ../include/broadcastdevice.hh ../include/lock.h
broadcastdevice.o: ../include/device.h ../include/playercommon.h
broadcastdevice.o: ../include/messages.h
clientdata.o: ../include/devicetable.h ../include/device.h ../include/lock.h
clientdata.o: ../include/playercommon.h ../include/clientdata.h
clientdata.o: ../include/messages.h ../include/counter.h ../include/packet.h
devicetable.o: ../include/devicetable.h ../include/device.h ../include/lock.h
devicetable.o: ../include/playercommon.h
gripperdevice.o: ../include/gripperdevice.h ../include/p2osdevice.h
gripperdevice.o: ../include/device.h ../include/lock.h
gripperdevice.o: ../include/playercommon.h ../include/packet.h
gripperdevice.o: ../include/messages.h ../include/sip.h
laserbeacondevice.o: ../include/laserbeacondevice.hh ../include/lock.h
laserbeacondevice.o: ../include/device.h ../include/playercommon.h
laserbeacondevice.o: ../include/messages.h ../include/devicetable.h
laserdevice.o: ../include/playercommon.h ../include/laserdevice.h
laserdevice.o: ../include/lock.h ../include/device.h ../include/messages.h
lock.o: ../include/device.h ../include/lock.h ../include/playercommon.h
lock.o: ../include/errors.h
main.o: ../include/pubsub_util.h ../include/clientdata.h
main.o: ../include/messages.h ../include/playercommon.h
main.o: ../include/devicetable.h ../include/device.h ../include/lock.h
main.o: ../include/counter.h ../include/nodevice.h
miscdevice.o: ../include/miscdevice.h ../include/p2osdevice.h
miscdevice.o: ../include/device.h ../include/lock.h ../include/playercommon.h
miscdevice.o: ../include/packet.h ../include/messages.h ../include/sip.h
nodevice.o: ../include/nodevice.h ../include/device.h ../include/lock.h
nodevice.o: ../include/playercommon.h
p2osdevice.o: ../include/p2osdevice.h ../include/device.h ../include/lock.h
p2osdevice.o: ../include/playercommon.h ../include/packet.h
p2osdevice.o: ../include/messages.h ../include/sip.h ../include/devicetable.h
packet.o: ../include/packet.h
positiondevice.o: ../include/positiondevice.h ../include/p2osdevice.h
positiondevice.o: ../include/device.h ../include/lock.h
positiondevice.o: ../include/playercommon.h ../include/packet.h
positiondevice.o: ../include/messages.h ../include/sip.h
ptzdevice.o: ../include/ptzdevice.h ../include/lock.h ../include/device.h
ptzdevice.o: ../include/playercommon.h ../include/messages.h
sip.o: ../include/sip.h ../include/messages.h ../include/playercommon.h
sip.o: ../include/p2osdevice.h ../include/device.h ../include/lock.h
sip.o: ../include/packet.h
sonardevice.o: ../include/sonardevice.h ../include/p2osdevice.h
sonardevice.o: ../include/device.h ../include/lock.h
sonardevice.o: ../include/playercommon.h ../include/packet.h
sonardevice.o: ../include/messages.h ../include/sip.h
speechdevice.o: ../include/speechdevice.h ../include/lock.h
speechdevice.o: ../include/device.h ../include/playercommon.h
speechdevice.o: ../include/messages.h ../include/pubsub_util.h
stagedevice.o: ../include/stagedevice.h ../include/playercommon.h
stagedevice.o: ../include/device.h ../include/lock.h ../include/arenalock.h
stagedevice.o: ../include/stage.h ../include/messages.h
visiondevice.o: ../include/visiondevice.h ../include/lock.h
visiondevice.o: ../include/device.h ../include/playercommon.h
visiondevice.o: ../include/messages.h ../include/pubsub_util.h
