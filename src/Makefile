#
# $Id$
#

.SUFFIXES: .cc .o

SERVERNAME = player

INCLUDES = -I../include
CPP = g++
CFLAGS = -g -Wall
LIBS = -lpthread -lm
MAKEDEP = makedepend
INSTALL = install
MKDIR = mkdir
RM = rm
RMDIR = rmdir

INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

OBS = nodevice.o clientdata.o laserdevice.o p2osdevice.o lock.o sonardevice.o main.o positiondevice.o pubsub_util.o visiondevice.o gripperdevice.o miscdevice.o packet.o sip.o arenapositiondevice.o arenalaserdevice.o arenasonardevice.o arenavisiondevice.o arenalock.o ptzdevice.o arenaptzdevice.o devicetable.o

all: $(SERVERNAME)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null

clean:
	rm -f *.o *~ $(SERVERNAME) core

$(SERVERNAME): $(OBS)
	$(CPP) $(OBS) -o $(SERVERNAME) $(LIBS)

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

arenalaserdevice.o: ../include/arenalaserdevice.h ../include/arenalock.h
arenalaserdevice.o: ../include/lock.h ../include/device.h
arenalaserdevice.o: ../include/offsets.h ../include/laserdevice.h
arenalock.o: ../include/device.h ../include/offsets.h ../include/lock.h
arenalock.o: ../include/arenalock.h ../include/errors.h
arenapositiondevice.o: ../include/arenapositiondevice.h
arenapositiondevice.o: ../include/arenalock.h ../include/lock.h
arenapositiondevice.o: ../include/device.h ../include/offsets.h
arenapositiondevice.o: ../include/positiondevice.h ../include/p2osdevice.h
arenapositiondevice.o: ../include/packet.h ../include/sip.h
arenaptzdevice.o: ../include/arenaptzdevice.h ../include/arenalock.h
arenaptzdevice.o: ../include/lock.h ../include/device.h ../include/offsets.h
arenaptzdevice.o: ../include/ptzdevice.h
arenasonardevice.o: ../include/arenasonardevice.h ../include/arenalock.h
arenasonardevice.o: ../include/lock.h ../include/device.h
arenasonardevice.o: ../include/offsets.h ../include/sonardevice.h
arenasonardevice.o: ../include/p2osdevice.h ../include/packet.h
arenasonardevice.o: ../include/sip.h
arenavisiondevice.o: ../include/arenavisiondevice.h ../include/arenalock.h
arenavisiondevice.o: ../include/lock.h ../include/device.h
arenavisiondevice.o: ../include/offsets.h ../include/visiondevice.h
clientdata.o: ../include/devicetable.h ../include/device.h
clientdata.o: ../include/offsets.h ../include/lock.h ../include/clientdata.h
clientdata.o: ../include/counter.h ../include/packet.h
devicetable.o: ../include/devicetable.h ../include/device.h
devicetable.o: ../include/offsets.h ../include/lock.h
gripperdevice.o: ../include/gripperdevice.h ../include/p2osdevice.h
gripperdevice.o: ../include/device.h ../include/offsets.h ../include/lock.h
gripperdevice.o: ../include/packet.h ../include/sip.h
laserdevice.o: ../include/laserdevice.h ../include/lock.h ../include/device.h
laserdevice.o: ../include/offsets.h
lock.o: ../include/device.h ../include/offsets.h ../include/lock.h
lock.o: ../include/errors.h
main.o: ../include/pubsub_util.h ../include/laserdevice.h ../include/lock.h
main.o: ../include/device.h ../include/offsets.h ../include/sonardevice.h
main.o: ../include/p2osdevice.h ../include/packet.h ../include/sip.h
main.o: ../include/visiondevice.h ../include/positiondevice.h
main.o: ../include/gripperdevice.h ../include/miscdevice.h
main.o: ../include/ptzdevice.h ../include/clientdata.h
main.o: ../include/devicetable.h ../include/counter.h ../include/nodevice.h
main.o: ../include/arenalaserdevice.h ../include/arenalock.h
main.o: ../include/arenasonardevice.h ../include/arenapositiondevice.h
main.o: ../include/arenavisiondevice.h ../include/arenaptzdevice.h
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
visiondevice.o: ../include/visiondevice.h ../include/lock.h
visiondevice.o: ../include/device.h ../include/offsets.h
visiondevice.o: ../include/pubsub_util.h
