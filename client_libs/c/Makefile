#
# $Id$
#

CC = gcc
RM = rm
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir
LN = ln
AR = ar

LIBNAME = libplayercclient.a

INSTALL_LIB = $(INSTALL_PREFIX)/lib
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

CFLAGS = -g -O2 -Wall 
INCLUDES = -I. -I../../include

OBJECTS = playercclient.o print.o helpers.o

all: $(LIBNAME)

$(LIBNAME): $(OBJECTS)
	ar cr $@ $(OBJECTS)

%.o: %.c playercclient.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

install: $(LIBNAME)
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 $(LIBNAME) $(INSTALL_LIB)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playercclient.h $(INSTALL_INCLUDE)

uninstall:
	$(RM) -f $(INSTALL_LIB)/$(LIBNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playercclient.h

clean:
	$(RM) -f $(LIBNAME) $(OBJECTS)
