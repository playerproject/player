#
# $Id$
#

include ../../Makefile.common

LIBNAME = libplayercclient.a

CFLAGS = -g3 -Wall $(PLAYER_PLATFORM)
INCLUDES = -I. -I../../include

OBJECTS = playercclient.o print.o helpers.o

all: $(LIBNAME)

$(LIBNAME): $(OBJECTS)
	ar cr $@ $(OBJECTS)

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.c 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak

%.o: %.c
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
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
