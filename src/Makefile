#
# $Id$
#
 
.SUFFIXES: .cc .o

SERVERNAME = player
DEVICELIBNAME = libplayerdevice.a

INCLUDES = -I../include
CPP = g++
CFLAGS = -g -Wall
LIBDIRS = -L.
LIBS = $(LIBDIRS) -lplayerdevice -lpthread -lm
MAKEDEP = makedepend
INSTALL = install
MKDIR = mkdir
RM = rm
AR = ar
RMDIR = rmdir

# comment to exclude support for the Stage simulator
INCLUDE_STAGE = -DINCLUDE_STAGE

# comment each one to remove support for that device
INCLUDE_LASER = -DINCLUDE_LASER
INCLUDE_SONAR = -DINCLUDE_SONAR
INCLUDE_VISION = -DINCLUDE_VISION
INCLUDE_POSITION = -DINCLUDE_POSITION
INCLUDE_GRIPPER = -DINCLUDE_GRIPPER
INCLUDE_MISC = -DINCLUDE_MISC
INCLUDE_PTZ = -DINCLUDE_PTZ
#INCLUDE_AUDIO = -DINCLUDE_AUDIO
# the following libs are only needed for the audio device
#EXTRA_LIBS = -lrfftw -lfftw
INCLUDE_LASERBEACON = -DINCLUDE_LASERBEACON
INCLUDE_BROADCAST = -DINCLUDE_BROADCAST

INCLUDED_DEVICES = $(INCLUDE_LASER) $(INCLUDE_SONAR) $(INCLUDE_VISION) \
                   $(INCLUDE_POSITION) $(INCLUDE_GRIPPER) $(INCLUDE_MISC) \
				   $(INCLUDE_MISC) $(INCLUDE_PTZ) $(INCLUDE_STAGE) \
                   $(INCLUDE_AUDIO) $(INCLUDE_LASERBEACON) $(INCLUDE_BROADCAST)

INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

OBS = nodevice.o clientdata.o laserdevice.o p2osdevice.o lock.o sonardevice.o positiondevice.o pubsub_util.o visiondevice.o gripperdevice.o miscdevice.o packet.o sip.o arenalock.o devicetable.o stagedevice.o ptzdevice.o laserbeacondevice.o broadcastdevice.o

all: $(SERVERNAME)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean:
	rm -f *.o *~ $(SERVERNAME) $(DEVICELIBNAME) core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): main.cc $(DEVICELIBNAME) 
	$(CPP) $(CFLAGS) $(INCLUDES) $(INCLUDED_DEVICES) -o $@ $< $(LIBS) $(EXTRA_LIBS)

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
laserdevice.o: ../include/playercommon.h ../include/laserdevice.h
laserdevice.o: ../include/lock.h ../include/device.h ../include/messages.h
lock.o: ../include/device.h ../include/lock.h ../include/playercommon.h
lock.o: ../include/errors.h
main.o: ../VERSION ../include/pubsub_util.h ../include/clientdata.h
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
p2osdevice.o: ../include/messages.h ../include/sip.h
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
stagedevice.o: ../include/stagedevice.h ../include/playercommon.h
stagedevice.o: ../include/device.h ../include/lock.h ../include/arenalock.h
stagedevice.o: ../include/stage.h ../include/messages.h
visiondevice.o: ../include/visiondevice.h ../include/lock.h
visiondevice.o: ../include/device.h ../include/playercommon.h
visiondevice.o: ../include/messages.h ../include/pubsub_util.h
