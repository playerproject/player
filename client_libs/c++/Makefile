#
# $Id$
#

include ../../Makefile.common

LIBNAME = libplayerclient.a

PWD=$(shell pwd)

INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o clientproxy.o laserproxy.o sonarproxy.o miscproxy.o \
      positionproxy.o ptzproxy.o gripperproxy.o speechproxy.o \
      laserbeaconproxy.o visionproxy.o truthproxy.o occupancyproxy.o \
      gpsproxy.o playermulticlient.o bpsproxy.o broadcastproxy.o moteproxy.o
      #idarproxy.o descartesproxy.o

all: $(LIBNAME)

C_CLIENT_DIR = ../c
C_CLIENT_OBJ = playercclient.o

$(LIBNAME): $(OBS) $(C_CLIENT_DIR)/$(C_CLIENT_OBJ)
	ar cr $@ $(OBS) ../c/playercclient.o
	$(MKDIR) -p ../../lib
	$(LN) -fs $(PWD)/$@ ../../lib
	$(MKDIR) -p ../../include
	$(LN) -fs $(PWD)/../c/playercclient.h ../../include
	$(LN) -fs $(PWD)/playerclient.h ../../include
	$(LN) -fs $(PWD)/clientproxy.h ../../include
	$(LN) -fs $(PWD)/gripperproxy.h ../../include
	$(LN) -fs $(PWD)/laserbeaconproxy.h ../../include
	$(LN) -fs $(PWD)/laserproxy.h ../../include
	$(LN) -fs $(PWD)/miscproxy.h ../../include
	$(LN) -fs $(PWD)/positionproxy.h ../../include
	$(LN) -fs $(PWD)/ptzproxy.h ../../include
	$(LN) -fs $(PWD)/sonarproxy.h ../../include
	$(LN) -fs $(PWD)/speechproxy.h ../../include
	$(LN) -fs $(PWD)/visionproxy.h ../../include
	$(LN) -fs $(PWD)/truthproxy.h ../../include
	$(LN) -fs $(PWD)/occupancyproxy.h ../../include
	$(LN) -fs $(PWD)/gpsproxy.h ../../include
	$(LN) -fs $(PWD)/bpsproxy.h ../../include
	$(LN) -fs $(PWD)/playermulticlient.h ../../include
	$(LN) -fs $(PWD)/broadcastproxy.h ../../include
	$(LN) -fs $(PWD)/moteproxy.h ../../include

$(C_CLIENT_DIR)/$(C_CLIENT_OBJ):
	cd $(C_CLIENT_DIR) && make $(C_CLIENT_OBJ)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak
	cd test && make dep

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak
	cd test && make clean_dep

%.o: %.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c $<

install: $(LIBNAME)
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 $(LIBNAME) $(INSTALL_LIB)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playerclient.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 clientproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 gripperproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 laserbeaconproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 laserproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 miscproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 positionproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 ptzproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 sonarproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 speechproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 visionproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 truthproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 occupancyproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 gpsproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 bpsproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playermulticlient.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 broadcastproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 moteproxy.h $(INSTALL_INCLUDE)
#	$(INSTALL) -m 644 idarproxy.h $(INSTALL_INCLUDE)
#	$(INSTALL) -m 644 descartesproxy.h $(INSTALL_INCLUDE)

uninstall:
	$(RM) -f $(INSTALL_LIB)/$(LIBNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playerclient.h
	$(RM) -f $(INSTALL_INCLUDE)/clientproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/gripperproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/laserbeaconproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/laserproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/miscproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/positionproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/ptzproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/sonarproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/speechproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/visionproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/truthproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/occupancyproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/gpsproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/bpsproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/broadcastproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/moteproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/playermulticlient.h
#	$(RM) -f $(INSTALL_INCLUDE)/descartesproxy.h
#	$(RM) -f $(INSTALL_INCLUDE)/idarproxy.h


clean:
	cd test && make clean
	$(RM) -f $(LIBNAME) *.o
	$(RM) -f ../../lib/$(LIBNAME)
	$(RM) -f ../../include/playercclient.h 
	$(RM) -f ../../include/playerclient.h 
	$(RM) -f ../../include/clientproxy.h 
	$(RM) -f ../../include/gripperproxy.h 
	$(RM) -f ../../include/laserbeaconproxy.h 
	$(RM) -f ../../include/laserproxy.h 
	$(RM) -f ../../include/miscproxy.h 
	$(RM) -f ../../include/positionproxy.h 
	$(RM) -f ../../include/ptzproxy.h 
	$(RM) -f ../../include/sonarproxy.h 
	$(RM) -f ../../include/speechproxy.h 
	$(RM) -f ../../include/visionproxy.h 
	$(RM) -f ../../include/truthproxy.h 
	$(RM) -f ../../include/occupancyproxy.h 
	$(RM) -f ../../include/gpsproxy.h 
	$(RM) -f ../../include/bpsproxy.h 
	$(RM) -f ../../include/moteproxy.h 
	$(RM) -f ../../include/broadcastproxy.h 
	$(RM) -f ../../include/playermulticlient.h 
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

bpsproxy.o: ./bpsproxy.h ./clientproxy.h ../c/playercclient.h
bpsproxy.o: ../../include/messages.h ../../include/playercommon.h
bpsproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
bpsproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
bpsproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
bpsproxy.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h
bpsproxy.o: ./broadcastproxy.h ./moteproxy.h
broadcastproxy.o: ./broadcastproxy.h ./clientproxy.h ../c/playercclient.h
broadcastproxy.o: ../../include/messages.h ../../include/playercommon.h
broadcastproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
broadcastproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
broadcastproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
broadcastproxy.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h
broadcastproxy.o: ./gpsproxy.h ./bpsproxy.h ./moteproxy.h
clientproxy.o: ./clientproxy.h ../c/playercclient.h ../../include/messages.h
clientproxy.o: ../../include/playercommon.h ../../include/defaults.h
clientproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
clientproxy.o: ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
clientproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
clientproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
clientproxy.o: ./broadcastproxy.h ./moteproxy.h
gpsproxy.o: ./gpsproxy.h ./clientproxy.h ../c/playercclient.h
gpsproxy.o: ../../include/messages.h ../../include/playercommon.h
gpsproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
gpsproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
gpsproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
gpsproxy.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h ./bpsproxy.h
gpsproxy.o: ./broadcastproxy.h ./moteproxy.h
gripperproxy.o: ./gripperproxy.h ./clientproxy.h ../c/playercclient.h
gripperproxy.o: ../../include/messages.h ../../include/playercommon.h
gripperproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
gripperproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
gripperproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
gripperproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
gripperproxy.o: ./broadcastproxy.h ./moteproxy.h
laserbeaconproxy.o: ./laserbeaconproxy.h ./clientproxy.h ../c/playercclient.h
laserbeaconproxy.o: ../../include/messages.h ../../include/playercommon.h
laserbeaconproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
laserbeaconproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h
laserbeaconproxy.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
laserbeaconproxy.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h
laserbeaconproxy.o: ./gpsproxy.h ./bpsproxy.h ./broadcastproxy.h
laserbeaconproxy.o: ./moteproxy.h
laserproxy.o: ./laserproxy.h ./clientproxy.h ../c/playercclient.h
laserproxy.o: ../../include/messages.h ../../include/playercommon.h
laserproxy.o: ../../include/defaults.h ./playerclient.h ./sonarproxy.h
laserproxy.o: ./miscproxy.h ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
laserproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
laserproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
laserproxy.o: ./broadcastproxy.h ./moteproxy.h
miscproxy.o: ./miscproxy.h ./clientproxy.h ../c/playercclient.h
miscproxy.o: ../../include/messages.h ../../include/playercommon.h
miscproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
miscproxy.o: ./sonarproxy.h ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
miscproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
miscproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
miscproxy.o: ./broadcastproxy.h ./moteproxy.h
moteproxy.o: ./playerclient.h ../../include/messages.h
moteproxy.o: ../../include/playercommon.h ../../include/defaults.h
moteproxy.o: ../c/playercclient.h ./laserproxy.h ./clientproxy.h
moteproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
moteproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
moteproxy.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h
moteproxy.o: ./bpsproxy.h ./broadcastproxy.h ./moteproxy.h
occupancyproxy.o: ./occupancyproxy.h ../../include/messages.h
occupancyproxy.o: ../../include/playercommon.h ../../include/defaults.h
occupancyproxy.o: ./clientproxy.h ../c/playercclient.h ./playerclient.h
occupancyproxy.o: ./laserproxy.h ./sonarproxy.h ./miscproxy.h
occupancyproxy.o: ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
occupancyproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
occupancyproxy.o: ./truthproxy.h ./gpsproxy.h ./bpsproxy.h ./broadcastproxy.h
occupancyproxy.o: ./moteproxy.h
playerclient.o: ./playerclient.h ../../include/messages.h
playerclient.o: ../../include/playercommon.h ../../include/defaults.h
playerclient.o: ../c/playercclient.h ./laserproxy.h ./clientproxy.h
playerclient.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
playerclient.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
playerclient.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h
playerclient.o: ./gpsproxy.h ./bpsproxy.h ./broadcastproxy.h ./moteproxy.h
playermulticlient.o: ./playermulticlient.h ./playerclient.h
playermulticlient.o: ../../include/messages.h ../../include/playercommon.h
playermulticlient.o: ../../include/defaults.h ../c/playercclient.h
playermulticlient.o: ./laserproxy.h ./clientproxy.h ./sonarproxy.h
playermulticlient.o: ./miscproxy.h ./positionproxy.h ./ptzproxy.h
playermulticlient.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
playermulticlient.o: ./visionproxy.h ./truthproxy.h ./occupancyproxy.h
playermulticlient.o: ./gpsproxy.h ./bpsproxy.h ./broadcastproxy.h
playermulticlient.o: ./moteproxy.h
positionproxy.o: ./positionproxy.h ./clientproxy.h ../c/playercclient.h
positionproxy.o: ../../include/messages.h ../../include/playercommon.h
positionproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
positionproxy.o: ./sonarproxy.h ./miscproxy.h ./ptzproxy.h ./gripperproxy.h
positionproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
positionproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
positionproxy.o: ./broadcastproxy.h ./moteproxy.h
ptzproxy.o: ./ptzproxy.h ./clientproxy.h ../c/playercclient.h
ptzproxy.o: ../../include/messages.h ../../include/playercommon.h
ptzproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
ptzproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./gripperproxy.h
ptzproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
ptzproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
ptzproxy.o: ./broadcastproxy.h ./moteproxy.h
sonarproxy.o: ./sonarproxy.h ./clientproxy.h ../c/playercclient.h
sonarproxy.o: ../../include/messages.h ../../include/playercommon.h
sonarproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
sonarproxy.o: ./miscproxy.h ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
sonarproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
sonarproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
sonarproxy.o: ./broadcastproxy.h ./moteproxy.h
speechproxy.o: ./speechproxy.h ./clientproxy.h ../c/playercclient.h
speechproxy.o: ../../include/messages.h ../../include/playercommon.h
speechproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
speechproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
speechproxy.o: ./gripperproxy.h ./laserbeaconproxy.h ./visionproxy.h
speechproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
speechproxy.o: ./broadcastproxy.h ./moteproxy.h
test.o: ./playerclient.h ../../include/messages.h
test.o: ../../include/playercommon.h ../../include/defaults.h
test.o: ../c/playercclient.h ./laserproxy.h ./clientproxy.h ./sonarproxy.h
test.o: ./miscproxy.h ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
test.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h ./truthproxy.h
test.o: ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h ./broadcastproxy.h
test.o: ./moteproxy.h
truthproxy.o: ./truthproxy.h ./clientproxy.h ../c/playercclient.h
truthproxy.o: ../../include/messages.h ../../include/playercommon.h
truthproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
truthproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
truthproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
truthproxy.o: ./visionproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
truthproxy.o: ./broadcastproxy.h ./moteproxy.h
visionproxy.o: ./visionproxy.h ./clientproxy.h ../c/playercclient.h
visionproxy.o: ../../include/messages.h ../../include/playercommon.h
visionproxy.o: ../../include/defaults.h ./playerclient.h ./laserproxy.h
visionproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
visionproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
visionproxy.o: ./truthproxy.h ./occupancyproxy.h ./gpsproxy.h ./bpsproxy.h
visionproxy.o: ./broadcastproxy.h ./moteproxy.h
