#
# $Id$
#

CPP= g++
RM = rm
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir
LN = ln
AR = ar

LIBNAME = libplayerclient.a

INSTALL_LIB = $(INSTALL_PREFIX)/lib
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include

CFLAGS = -g -O2 -Wall 
INCLUDES = -I. -I../../include

all: $(LIBNAME)

$(LIBNAME): playerclient.cc playerclient.h
	$(CPP) $(CFLAGS) $(INCLUDES) -c -o playerclient.o $<
	ar cr $@ playerclient.o

install: $(LIBNAME)
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 $(LIBNAME) $(INSTALL_LIB)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playerclient.h $(INSTALL_INCLUDE)

uninstall:
	$(RM) -f $(INSTALL_LIB)/$(LIBNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playerclient.h

clean:
	$(RM) -f $(LIBNAME) *.o
