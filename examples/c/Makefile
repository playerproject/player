#
# $Id$
#

CC = gcc
RM = rm
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir

INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/c

CFLAGS = -g -O2 -Wall $(PLAYER_PLATFORM)
INCLUDES = -I../../client_libs/c -I../../include -I$(INSTALL_PREFIX)/include -I/usr/local/player/include
LIBDIRS = -L../../client_libs/c -L$(INSTALL_PREFIX)/lib -L/usr/local/player/lib
LIBS = $(LIBDIRS) -lplayercclient $(PLATFORM_LIBS)

BINARIES = randomwalk laserobstacleavoid ptz 

all: $(BINARIES)

test: test.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ test.c $(LIBS)

randomwalk: randomwalk.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ randomwalk.c $(LIBS)

laserobstacleavoid: laserobstacleavoid.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ laserobstacleavoid.c $(LIBS)

ptz: ptz.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ ptz.c $(LIBS)

install: $(BINARIES)
	$(MKDIR) -p $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 Makefile $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 randomwalk.c $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 laserobstacleavoid.c $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 ptz.c $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 ptz $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 randomwalk $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 laserobstacleavoid $(INSTALL_EXAMPLES)

uninstall:
	$(RM) -f $(INSTALL_EXAMPLES)/Makefile
	$(RM) -f $(INSTALL_EXAMPLES)/randomwalk.c
	$(RM) -f $(INSTALL_EXAMPLES)/laserobstacleavoid.c
	$(RM) -f $(INSTALL_EXAMPLES)/ptz.c
	$(RM) -f $(INSTALL_EXAMPLES)/randomwalk
	$(RM) -f $(INSTALL_EXAMPLES)/laserobstacleavoid
	$(RM) -f $(INSTALL_EXAMPLES)/ptz

clean:
	$(RM) -f *.o $(BINARIES)
