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

LIBDIRS = -L.
LIBS = $(LIBDIRS) -lplayerdevice -lpthread -lm -ldl

INCLUDED_DEVICES = $(INCLUDE_LASER) $(INCLUDE_SONAR) $(INCLUDE_VISION) \
                   $(INCLUDE_POSITION) $(INCLUDE_GRIPPER) $(INCLUDE_MISC) \
                   $(INCLUDE_MISC) $(INCLUDE_PTZ) $(INCLUDE_STAGE) \
                   $(INCLUDE_AUDIO) $(INCLUDE_LASERBEACON) $(INCLUDE_BPS) \
                   $(INCLUDE_BROADCAST) $(INCLUDE_SPEECH)

#
# definitely build the device-independent objects
#
OBS = clientdata.o pubsub_util.o devicetable.o device.o \
      clientmanager.o wallclocktime.o main.o playerqueue.o

#
# only build the device-specific objects that we need (duplicate objects
# won't cause problems)
#
#ifdef INCLUDE_STAGE
#  OBS += truthdevice.o
#endif
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

#SHARED_OBJS = laser.so

all: $(SERVERNAME) $(SHARED_OBJS)

%.so: %.o 
	$(CPP) -shared -nostartfiles -o $@ $<

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak
clean:
	rm -f *.so *.o *~ $(SERVERNAME) $(DEVICELIBNAME) core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): $(DEVICELIBNAME) 
	$(CPP) -rdynamic -o $@ $(LIBS) $(EXTRA_LIBS) $(PLATFORM_LIBS)

main.o: main.cc
	$(CPP) $(CFLAGS) -DPLAYER_VERSION=\"$(PLAYER_VERSION)\" $(INCLUDES) $(INCLUDED_DEVICES) -o $@ -c main.cc 

%.o: %.cc 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<


install: $(SERVERNAME)
	$(MKDIR) -p $(INSTALL_BIN)
	$(INSTALL) -m 755 $(SERVERNAME) $(INSTALL_BIN)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 ../include/playercommon.h $(INSTALL_INCLUDE)/playercommon.h
	$(INSTALL) -m 644 ../include/messages.h $(INSTALL_INCLUDE)/messages.h
	$(INSTALL) -m 644 ../include/defaults.h $(INSTALL_INCLUDE)/defaults.h
	$(INSTALL) -m 644 ../include/stage.h $(INSTALL_INCLUDE)/stage.h

uninstall:
	$(RM) -f $(INSTALL_BIN)/$(SERVERNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playercommon.h
	$(RM) -f $(INSTALL_INCLUDE)/messages.h
	$(RM) -f $(INSTALL_INCLUDE)/defaults.h
	$(RM) -f $(INSTALL_INCLUDE)/stage.h
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

audiodevice.o: ../include/audiodevice.h ../include/device.h
audiodevice.o: ../include/playercommon.h ../include/playerqueue.h
audiodevice.o: ../include/clientdata.h ../include/messages.h
audiodevice.o: ../include/defaults.h
bpsdevice.o: ../include/bpsdevice.h ../include/device.h
bpsdevice.o: ../include/playercommon.h ../include/playerqueue.h
bpsdevice.o: ../include/clientdata.h ../include/messages.h
bpsdevice.o: ../include/defaults.h ../include/devicetable.h
bpsdevice.o: ../include/playertime.h
broadcastdevice.o: ../include/broadcastdevice.hh ../include/device.h
broadcastdevice.o: ../include/playercommon.h ../include/playerqueue.h
broadcastdevice.o: ../include/clientdata.h ../include/messages.h
broadcastdevice.o: ../include/defaults.h ../include/playertime.h
clientdata.o: ../include/device.h ../include/playercommon.h
clientdata.o: ../include/playerqueue.h ../include/clientdata.h
clientdata.o: ../include/messages.h ../include/defaults.h
clientdata.o: ../include/devicetable.h ../include/clientmanager.h
clientdata.o: ../include/packet.h ../include/playertime.h
clientmanager.o: ../include/clientmanager.h ../include/clientdata.h
clientmanager.o: ../include/messages.h ../include/playercommon.h
clientmanager.o: ../include/defaults.h ../include/device.h
clientmanager.o: ../include/playerqueue.h ../include/playertime.h
device.o: ../include/device.h ../include/playercommon.h
device.o: ../include/playerqueue.h ../include/clientdata.h
device.o: ../include/messages.h ../include/defaults.h ../include/playertime.h
devicetable.o: ../include/devicetable.h ../include/device.h
devicetable.o: ../include/playercommon.h ../include/playerqueue.h
devicetable.o: ../include/clientdata.h ../include/messages.h
devicetable.o: ../include/defaults.h
gripperdevice.o: ../include/gripperdevice.h ../include/p2osdevice.h
gripperdevice.o: ../include/device.h ../include/playercommon.h
gripperdevice.o: ../include/playerqueue.h ../include/clientdata.h
gripperdevice.o: ../include/messages.h ../include/defaults.h
gripperdevice.o: ../include/packet.h ../include/robot_params.h
gripperdevice.o: ../include/sip.h
laserbeacondevice.o: ../include/laserbeacondevice.hh ../include/device.h
laserbeacondevice.o: ../include/playercommon.h ../include/playerqueue.h
laserbeacondevice.o: ../include/clientdata.h ../include/messages.h
laserbeacondevice.o: ../include/defaults.h ../include/devicetable.h
laserdevice.o: ../include/playertime.h ../include/playercommon.h
laserdevice.o: ../include/laserdevice.h ../include/device.h
laserdevice.o: ../include/playerqueue.h ../include/clientdata.h
laserdevice.o: ../include/messages.h ../include/defaults.h
main.o: ../include/pubsub_util.h ../include/defaults.h
main.o: ../include/clientdata.h ../include/messages.h
main.o: ../include/playercommon.h ../include/clientmanager.h
main.o: ../include/devicetable.h ../include/device.h ../include/playerqueue.h
main.o: ../include/playertime.h ../include/wallclocktime.h
main.o: ../include/stagetime.h ../include/stage.h
miscdevice.o: ../include/miscdevice.h ../include/p2osdevice.h
miscdevice.o: ../include/device.h ../include/playercommon.h
miscdevice.o: ../include/playerqueue.h ../include/clientdata.h
miscdevice.o: ../include/messages.h ../include/defaults.h ../include/packet.h
miscdevice.o: ../include/robot_params.h ../include/sip.h
p2osdevice.o: ../include/p2osdevice.h ../include/device.h
p2osdevice.o: ../include/playercommon.h ../include/playerqueue.h
p2osdevice.o: ../include/clientdata.h ../include/messages.h
p2osdevice.o: ../include/defaults.h ../include/packet.h
p2osdevice.o: ../include/robot_params.h ../include/sip.h
p2osdevice.o: ../include/playertime.h ../include/devicetable.h
packet.o: ../include/packet.h ../include/playertime.h
playerqueue.o: ../include/playerqueue.h ../include/clientdata.h
playerqueue.o: ../include/messages.h ../include/playercommon.h
playerqueue.o: ../include/defaults.h ../include/playertime.h
positiondevice.o: ../include/positiondevice.h ../include/p2osdevice.h
positiondevice.o: ../include/device.h ../include/playercommon.h
positiondevice.o: ../include/playerqueue.h ../include/clientdata.h
positiondevice.o: ../include/messages.h ../include/defaults.h
positiondevice.o: ../include/packet.h ../include/robot_params.h
positiondevice.o: ../include/sip.h
ptzdevice.o: ../include/ptzdevice.h ../include/device.h
ptzdevice.o: ../include/playercommon.h ../include/playerqueue.h
ptzdevice.o: ../include/clientdata.h ../include/messages.h
ptzdevice.o: ../include/defaults.h
robot_params.o: ../include/robot_params.h
sip.o: ../include/sip.h ../include/messages.h ../include/playercommon.h
sip.o: ../include/defaults.h ../include/p2osdevice.h ../include/device.h
sip.o: ../include/playerqueue.h ../include/clientdata.h ../include/packet.h
sip.o: ../include/robot_params.h
sonardevice.o: ../include/sonardevice.h ../include/p2osdevice.h
sonardevice.o: ../include/device.h ../include/playercommon.h
sonardevice.o: ../include/playerqueue.h ../include/clientdata.h
sonardevice.o: ../include/messages.h ../include/defaults.h
sonardevice.o: ../include/packet.h ../include/robot_params.h ../include/sip.h
speechdevice.o: ../include/speechdevice.h ../include/device.h
speechdevice.o: ../include/playercommon.h ../include/playerqueue.h
speechdevice.o: ../include/clientdata.h ../include/messages.h
speechdevice.o: ../include/defaults.h ../include/pubsub_util.h
stagedevice.o: ../include/stagedevice.h ../include/playercommon.h
stagedevice.o: ../include/device.h ../include/playerqueue.h
stagedevice.o: ../include/clientdata.h ../include/messages.h
stagedevice.o: ../include/defaults.h ../include/stage.h
stagedevice.o: ../include/playertime.h
stagetime.o: ../include/stagetime.h ../include/playertime.h
stagetime.o: ../include/stage.h ../include/messages.h
stagetime.o: ../include/playercommon.h ../include/defaults.h
visiondevice.o: ../include/visiondevice.h ../include/device.h
visiondevice.o: ../include/playercommon.h ../include/playerqueue.h
visiondevice.o: ../include/clientdata.h ../include/messages.h
visiondevice.o: ../include/defaults.h ../include/pubsub_util.h
wallclocktime.o: ../include/wallclocktime.h ../include/playertime.h
