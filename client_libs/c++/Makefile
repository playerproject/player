#
# $Id$
#

include ../../Makefile.common

LIBNAME = libplayerclient.a

PWD=$(shell pwd)

INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o clientproxy.o laserproxy.o sonarproxy.o miscproxy.o \
      positionproxy.o ptzproxy.o gripperproxy.o speechproxy.o \
      laserbeaconproxy.o visionproxy.o  \
      gpsproxy.o playermulticlient.o bpsproxy.o broadcastproxy.o moteproxy.o \
      rwi_positionproxy.o rwi_sonarproxy.o rwi_laserproxy.o rwi_bumperproxy.o \
      rwi_powerproxy.o truthproxy.o descartesproxy.o idarproxy.o 

all: $(LIBNAME)

C_CLIENT_DIR = ../c
C_CLIENT_OBJ = playercclient.o

$(LIBNAME): $(OBS) $(C_CLIENT_DIR)/$(C_CLIENT_OBJ)
	ar cr $@ $(OBS) ../c/playercclient.o
	$(MKDIR) -p ../../lib
	$(RM) -f ../../lib/$@
	$(LN) -s $(PWD)/$@ ../../lib
	$(MKDIR) -p ../../include
	$(RM) -f ../../include/playercclient.h 
	$(LN) -s $(PWD)/../c/playercclient.h ../../include
	$(RM) -f ../../include/playerclient.h 
	$(LN) -s $(PWD)/playerclient.h ../../include
	$(RM) -f ../../include/clientproxy.h 
	$(LN) -s $(PWD)/clientproxy.h ../../include
	$(RM) -f ../../include/gripperproxy.h 
	$(LN) -s $(PWD)/gripperproxy.h ../../include
	$(RM) -f ../../include/laserbeaconproxy.h 
	$(LN) -s $(PWD)/laserbeaconproxy.h ../../include
	$(RM) -f ../../include/laserproxy.h 
	$(LN) -s $(PWD)/laserproxy.h ../../include
	$(RM) -f ../../include/miscproxy.h 
	$(LN) -s $(PWD)/miscproxy.h ../../include
	$(RM) -f ../../include/positionproxy.h 
	$(LN) -s $(PWD)/positionproxy.h ../../include
	$(RM) -f ../../include/ptzproxy.h 
	$(LN) -s $(PWD)/ptzproxy.h ../../include
	$(RM) -f ../../include/sonarproxy.h 
	$(LN) -s $(PWD)/sonarproxy.h ../../include
	$(RM) -f ../../include/speechproxy.h 
	$(LN) -s $(PWD)/speechproxy.h ../../include
	$(RM) -f ../../include/visionproxy.h 
	$(LN) -s $(PWD)/visionproxy.h ../../include
	$(RM) -f ../../include/truthproxy.h 
	$(LN) -s $(PWD)/truthproxy.h ../../include
	$(RM) -f ../../include/occupancyproxy.h 
	$(LN) -s $(PWD)/occupancyproxy.h ../../include
	$(RM) -f ../../include/gpsproxy.h 
	$(LN) -s $(PWD)/gpsproxy.h ../../include
	$(RM) -f ../../include/bpsproxy.h 
	$(LN) -s $(PWD)/bpsproxy.h ../../include
	$(RM) -f ../../include/playermulticlient.h 
	$(LN) -s $(PWD)/playermulticlient.h ../../include
	$(RM) -f ../../include/broadcastproxy.h 
	$(LN) -s $(PWD)/broadcastproxy.h ../../include
	$(RM) -f ../../include/moteproxy.h 
	$(LN) -s $(PWD)/moteproxy.h ../../include
	$(RM) -f ../../include/rwi_positionproxy.h 
	$(LN) -s $(PWD)/rwi_positionproxy.h ../../include
	$(RM) -f ../../include/rwi_sonarproxy.h 
	$(LN) -s $(PWD)/rwi_sonarproxy.h ../../include
	$(RM) -f ../../include/rwi_laserproxy.h 
	$(LN) -s $(PWD)/rwi_laserproxy.h ../../include
	$(RM) -f ../../include/rwi_bumperproxy.h 
	$(LN) -s $(PWD)/rwi_bumperproxy.h ../../include
	$(RM) -f ../../include/rwi_powerproxy.h
	$(LN) -s $(PWD)/rwi_powerproxy.h ../../include
	$(RM) -f ../../include/descartesproxy.h 
	$(LN) -s $(PWD)/descartesproxy.h ../../include
	$(RM) -f ../../include/idarproxy.h 
	$(LN) -s $(PWD)/idarproxy.h ../../include

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
	$(INSTALL) -m 644 rwi_positionproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 rwi_sonarproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 rwi_laserproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 rwi_bumperproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 rwi_powerproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 idarproxy.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 descartesproxy.h $(INSTALL_INCLUDE)

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
	$(RM) -f $(INSTALL_INCLUDE)/rwi_positionproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/rwi_sonarproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/rwi_laserproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/rwi_bumperproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/rwi_powerproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/descartesproxy.h
	$(RM) -f $(INSTALL_INCLUDE)/idarproxy.h


clean:
	cd test && make clean
	$(RM) -f $(LIBNAME) *.o *~
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
	$(RM) -f ../../include/rwi_positionproxy.h
	$(RM) -f ../../include/rwi_sonarproxy.h
	$(RM) -f ../../include/rwi_laserproxy.h
	$(RM) -f ../../include/rwi_bumperproxy.h
	$(RM) -f ../../include/rwi_powerproxy.h
	$(RM) -f ../../include/descartesproxy.h 
	$(RM) -f ../../include/idarproxy.h 
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
