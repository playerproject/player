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
      gpsproxy.o playermulticlient.o bpsproxy.o broadcastproxy.o 
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
	$(LN) -fs $(PWD)/broadcastproxy.h ../../include
	$(LN) -fs $(PWD)/playermulticlient.h ../../include

$(C_CLIENT_DIR)/$(C_CLIENT_OBJ):
	cd $(C_CLIENT_DIR) && make $(C_CLIENT_OBJ)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
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
	$(INSTALL) -m 644 bpsproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 broadcastproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playermulticlient.h $(INSTALL_INCLUDE)
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
	$(RM) -f $(INSTALL_INCLUDE)/playermulticlient.h
#	$(RM) -f $(INSTALL_INCLUDE)/idarproxy.h
#	$(RM) -f $(INSTALL_INCLUDE)/descartesproxy.h


clean:
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
	$(RM) -f ../../include/broadcastproxy.h 
	$(RM) -f ../../include/playermulticlient.h 
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
