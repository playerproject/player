#
# $Id$
#

include ../../Makefile.common

LIBNAME = libplayerclient.a

CFLAGS = -g3 -Wall $(PLAYER_PLATFORM)
INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o clientproxy.o laserproxy.o sonarproxy.o miscproxy.o \
      positionproxy.o ptzproxy.o gripperproxy.o speechproxy.o \
      laserbeaconproxy.o visionproxy.o truthproxy.o occupancyproxy.o \
      gpsproxy.o playermulticlient.o

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
	$(INSTALL) -m 644 truthproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 occupancyproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 gpsproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playermulticlient.h $(INSTALL_INCLUDE)

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
	$(RM) -f $(INSTALL_INCLUDE)/playermulticlient.h

clean:
	$(RM) -f $(LIBNAME) *.o
