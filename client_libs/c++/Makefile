#
# $Id$
#

include ../../Makefile.common

LIBNAME = libplayerclient.a

CFLAGS = -g3 -Wall $(PLAYER_PLATFORM)
INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o clientproxy.o laserproxy.o sonarproxy.o miscproxy.o \
      positionproxy.o ptzproxy.o gripperproxy.o speechproxy.o \
      laserbeaconproxy.o visionproxy.o gpsproxy.o playermulticlient.o

all: $(LIBNAME)

C_CLIENT_DIR = ../c
C_CLIENT_OBJ = playercclient.o

$(LIBNAME): $(OBS) $(C_CLIENT_DIR)/$(C_CLIENT_OBJ)
	ar cr $@ $(OBS) ../c/playercclient.o

$(C_CLIENT_DIR)/$(C_CLIENT_OBJ):
	cd $(C_CLIENT_DIR) && make $(C_CLIENT_OBJ)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

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

clean:
	$(RM) -f $(LIBNAME) *.o
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

clientproxy.o: ./clientproxy.h ../c/playercclient.h ../../include/messages.h
clientproxy.o: ../../include/playercommon.h ./playerclient.h ./laserproxy.h
clientproxy.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h ./ptzproxy.h
clientproxy.o: ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
clientproxy.o: ./visionproxy.h ./gpsproxy.h
gpsproxy.o: ./gpsproxy.h ./clientproxy.h ../c/playercclient.h
gpsproxy.o: ../../include/messages.h ../../include/playercommon.h
gpsproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
gpsproxy.o: ./positionproxy.h ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
gpsproxy.o: ./laserbeaconproxy.h ./visionproxy.h
gripperproxy.o: ./gripperproxy.h ./clientproxy.h ../c/playercclient.h
gripperproxy.o: ../../include/messages.h ../../include/playercommon.h
gripperproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
gripperproxy.o: ./positionproxy.h ./ptzproxy.h ./speechproxy.h
gripperproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
laserbeaconproxy.o: ./laserbeaconproxy.h ./clientproxy.h ../c/playercclient.h
laserbeaconproxy.o: ../../include/messages.h ../../include/playercommon.h
laserbeaconproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h
laserbeaconproxy.o: ./miscproxy.h ./positionproxy.h ./ptzproxy.h
laserbeaconproxy.o: ./gripperproxy.h ./speechproxy.h ./visionproxy.h
laserbeaconproxy.o: ./gpsproxy.h
laserproxy.o: ./laserproxy.h ./clientproxy.h ../c/playercclient.h
laserproxy.o: ../../include/messages.h ../../include/playercommon.h
laserproxy.o: ./playerclient.h ./sonarproxy.h ./miscproxy.h ./positionproxy.h
laserproxy.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
laserproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
miscproxy.o: ./miscproxy.h ./clientproxy.h ../c/playercclient.h
miscproxy.o: ../../include/messages.h ../../include/playercommon.h
miscproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./positionproxy.h
miscproxy.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
miscproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
playerclient.o: ./playerclient.h ../../include/messages.h
playerclient.o: ../../include/playercommon.h ../c/playercclient.h
playerclient.o: ./laserproxy.h ./clientproxy.h ./sonarproxy.h ./miscproxy.h
playerclient.o: ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
playerclient.o: ./speechproxy.h ./laserbeaconproxy.h ./visionproxy.h
playerclient.o: ./gpsproxy.h
playermulticlient.o: ./playermulticlient.h ./playerclient.h
playermulticlient.o: ../../include/messages.h ../../include/playercommon.h
playermulticlient.o: ../c/playercclient.h ./laserproxy.h ./clientproxy.h
playermulticlient.o: ./sonarproxy.h ./miscproxy.h ./positionproxy.h
playermulticlient.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
playermulticlient.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
positionproxy.o: ./positionproxy.h ./clientproxy.h ../c/playercclient.h
positionproxy.o: ../../include/messages.h ../../include/playercommon.h
positionproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
positionproxy.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
positionproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
ptzproxy.o: ./ptzproxy.h ./clientproxy.h ../c/playercclient.h
ptzproxy.o: ../../include/messages.h ../../include/playercommon.h
ptzproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
ptzproxy.o: ./positionproxy.h ./gripperproxy.h ./speechproxy.h
ptzproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
sonarproxy.o: ./sonarproxy.h ./clientproxy.h ../c/playercclient.h
sonarproxy.o: ../../include/messages.h ../../include/playercommon.h
sonarproxy.o: ./playerclient.h ./laserproxy.h ./miscproxy.h ./positionproxy.h
sonarproxy.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h
sonarproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
speechproxy.o: ./speechproxy.h ./clientproxy.h ../c/playercclient.h
speechproxy.o: ../../include/messages.h ../../include/playercommon.h
speechproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
speechproxy.o: ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
speechproxy.o: ./laserbeaconproxy.h ./visionproxy.h ./gpsproxy.h
test.o: ./playerclient.h ../../include/messages.h
test.o: ../../include/playercommon.h ../c/playercclient.h ./laserproxy.h
test.o: ./clientproxy.h ./sonarproxy.h ./miscproxy.h ./positionproxy.h
test.o: ./ptzproxy.h ./gripperproxy.h ./speechproxy.h ./laserbeaconproxy.h
test.o: ./visionproxy.h ./gpsproxy.h
visionproxy.o: ./visionproxy.h ./clientproxy.h ../c/playercclient.h
visionproxy.o: ../../include/messages.h ../../include/playercommon.h
visionproxy.o: ./playerclient.h ./laserproxy.h ./sonarproxy.h ./miscproxy.h
visionproxy.o: ./positionproxy.h ./ptzproxy.h ./gripperproxy.h
visionproxy.o: ./speechproxy.h ./laserbeaconproxy.h ./gpsproxy.h
