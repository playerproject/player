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

INCLUDED_DEVICES = $(INCLUDE_LASER) $(INCLUDE_SONAR) $(INCLUDE_VISION) \
                   $(INCLUDE_POSITION) $(INCLUDE_GRIPPER) $(INCLUDE_MISC) \
		   $(INCLUDE_MISC) $(INCLUDE_PTZ) $(INCLUDE_STAGE)

INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

OBS = nodevice.o clientdata.o laserdevice.o p2osdevice.o lock.o sonardevice.o positiondevice.o pubsub_util.o visiondevice.o gripperdevice.o miscdevice.o packet.o sip.o arenalock.o devicetable.o stagedevice.o ptzdevice.o 

all: $(SERVERNAME)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean:
	rm -f *.o *~ $(SERVERNAME) $(DEVICELIBNAME) core

$(DEVICELIBNAME): $(OBS)
	$(AR) cr $@ $(OBS)

$(SERVERNAME): main.cc $(DEVICELIBNAME) 
	$(CPP) $(CFLAGS) $(INCLUDES) $(INCLUDED_DEVICES) -o $@ $< $(LIBS)

%.o: %.cc 
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<


install: $(SERVERNAME)
	$(MKDIR) -p $(INSTALL_BIN)
	$(INSTALL) -m 755 $(SERVERNAME) $(INSTALL_BIN)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 ../include/offsets.h $(INSTALL_INCLUDE)/offsets.h

uninstall:
	$(RM) -f $(INSTALL_BIN)/$(SERVERNAME)
	$(RM) -f $(INSTALL_INCLUDE)/offsets.h

# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

arenalock.o: ../include/device.h ../include/offsets.h ../include/lock.h
arenalock.o: ../include/arenalock.h ../include/errors.h
clientdata.o: ../include/devicetable.h ../include/device.h
clientdata.o: ../include/offsets.h ../include/lock.h ../include/clientdata.h
clientdata.o: ../include/counter.h ../include/packet.h
devicetable.o: ../include/devicetable.h ../include/device.h
devicetable.o: ../include/offsets.h ../include/lock.h
gripperdevice.o: ../include/gripperdevice.h ../include/p2osdevice.h
gripperdevice.o: ../include/device.h ../include/offsets.h ../include/lock.h
gripperdevice.o: ../include/packet.h ../include/sip.h
laserdevice.o: ../include/rtk-types.hh ../include/laserdevice.h
laserdevice.o: ../include/lock.h ../include/device.h ../include/offsets.h
lock.o: ../include/device.h ../include/offsets.h ../include/lock.h
lock.o: ../include/errors.h
main.o: ../VERSION ../include/pubsub_util.h ../include/clientdata.h
main.o: ../include/devicetable.h ../include/device.h ../include/offsets.h
main.o: ../include/lock.h ../include/counter.h ../include/nodevice.h
miscdevice.o: ../include/miscdevice.h ../include/p2osdevice.h
miscdevice.o: ../include/device.h ../include/offsets.h ../include/lock.h
miscdevice.o: ../include/packet.h ../include/sip.h
nodevice.o: ../include/nodevice.h ../include/device.h ../include/offsets.h
nodevice.o: ../include/lock.h
p2osdevice.o: ../include/p2osdevice.h ../include/device.h
p2osdevice.o: ../include/offsets.h ../include/lock.h ../include/packet.h
p2osdevice.o: ../include/sip.h
packet.o: ../include/packet.h
positiondevice.o: ../include/positiondevice.h ../include/p2osdevice.h
positiondevice.o: ../include/device.h ../include/offsets.h ../include/lock.h
positiondevice.o: ../include/packet.h ../include/sip.h
ptzdevice.o: ../include/ptzdevice.h ../include/lock.h ../include/device.h
ptzdevice.o: ../include/offsets.h
sip.o: ../include/sip.h ../include/p2osdevice.h ../include/device.h
sip.o: ../include/offsets.h ../include/lock.h ../include/packet.h
sonardevice.o: ../include/sonardevice.h ../include/p2osdevice.h
sonardevice.o: ../include/device.h ../include/offsets.h ../include/lock.h
sonardevice.o: ../include/packet.h ../include/sip.h
stagedevice.o: ../include/stagedevice.hh ../include/rtk-types.hh
stagedevice.o: ../include/device.h ../include/offsets.h ../include/lock.h
stagedevice.o: ../include/arenalock.h
visiondevice.o: ../include/visiondevice.h ../include/lock.h
visiondevice.o: ../include/device.h ../include/offsets.h
visiondevice.o: ../include/pubsub_util.h
