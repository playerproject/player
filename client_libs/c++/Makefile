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
INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o devicedatatable.o

all: $(LIBNAME)

$(LIBNAME): $(OBS)
	ar cr $@ $(OBS) ../c/playercclient.o

%.o: %.cc %.h
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c $<

install: $(LIBNAME)
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 $(LIBNAME) $(INSTALL_LIB)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 playerclient.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 devicedatatable.h $(INSTALL_INCLUDE)

uninstall:
	$(RM) -f $(INSTALL_LIB)/$(LIBNAME)
	$(RM) -f $(INSTALL_INCLUDE)/playerclient.h
	$(RM) -f $(INSTALL_INCLUDE)/devicedatatable.h

clean:
	$(RM) -f $(LIBNAME) *.o
