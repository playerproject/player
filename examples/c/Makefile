#
# $Id$
#

CC = gcc
RM = rm
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir

INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/c

CFLAGS = -g -O2 -Wall 
INCLUDES = -I../../client_libs/c -I../../include -I/usr/local/player/include 
LIBDIRS = -L../../client_libs/c -L/usr/local/player/lib 
LIBS = $(LIBDIRS) -lplayercclient

BINARIES = randomwalk laserobstacleavoid ptz test

all: $(BINARIES)

test: test.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

randomwalk: randomwalk.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

laserobstacleavoid: laserobstacleavoid.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

ptz: ptz.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

clean:
	$(RM) -f *.o $(BINARIES)
