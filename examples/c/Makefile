#
# $Id$
#

include ../../Makefile.common

INCLUDES = -I../../include -I$(INSTALL_PREFIX)/include \
           -I/usr/local/player/include
LIBDIRS = -L../../lib -L$(INSTALL_PREFIX)/lib \
          -L/usr/local/player/lib
LIBS = $(LIBDIRS) -lplayercclient $(PLATFORM_LIBS)

BINARIES = randomwalk laserobstacleavoid ptz 

all: $(BINARIES)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.c 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak

test: test.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LIBS)

randomwalk: randomwalk.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ randomwalk.c $(LIBS)

laserobstacleavoid: laserobstacleavoid.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ laserobstacleavoid.c $(LIBS)

ptz: ptz.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ ptz.c $(LIBS)

install: $(BINARIES)
	$(MKDIR) -p $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 644 Makefile $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 644 randomwalk.c $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 644 laserobstacleavoid.c $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 644 ptz.c $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 755 ptz $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 755 randomwalk $(INSTALL_EXAMPLES)/c
	$(INSTALL) -m 755 laserobstacleavoid $(INSTALL_EXAMPLES)/c

uninstall:
	$(RM) -f $(INSTALL_EXAMPLES)/c/Makefile
	$(RM) -f $(INSTALL_EXAMPLES)/c/randomwalk.c
	$(RM) -f $(INSTALL_EXAMPLES)/c/laserobstacleavoid.c
	$(RM) -f $(INSTALL_EXAMPLES)/c/ptz.c
	$(RM) -f $(INSTALL_EXAMPLES)/c/randomwalk
	$(RM) -f $(INSTALL_EXAMPLES)/c/laserobstacleavoid
	$(RM) -f $(INSTALL_EXAMPLES)/c/ptz

clean:
	$(RM) -f *.o $(BINARIES)
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

laserobstacleavoid.o: ../../include/playercclient.h ../../include/messages.h
laserobstacleavoid.o: ../../include/playercommon.h ../../include/defaults.h
ptz.o: ../../include/playercclient.h ../../include/messages.h
ptz.o: ../../include/playercommon.h ../../include/defaults.h
randomwalk.o: ../../include/playercclient.h ../../include/messages.h
randomwalk.o: ../../include/playercommon.h ../../include/defaults.h
