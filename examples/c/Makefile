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

BINARIES = simple

all: $(BINARIES)

simple: simple.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

clean:
	$(RM) -f *.o $(BINARIES)
