#
# $Id$
#

include ../../Makefile.common

LIBNAME = liboldplayerclient.a

INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o devicedatatable.o

all: $(LIBNAME)

$(LIBNAME): $(OBS)
	ar cr $@ $(OBS) ../c/playercclient.o

%.o: %.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c $<

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak

install: $(LIBNAME)
	$(MKDIR) -p $(INSTALL_LIB)
	$(INSTALL) -m 644 $(LIBNAME) $(INSTALL_LIB)
	$(MKDIR) -p $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 oldplayerclient.h $(INSTALL_INCLUDE)
	$(INSTALL) -m 644 olddevicedatatable.h $(INSTALL_INCLUDE)

uninstall:
	$(RM) -f $(INSTALL_LIB)/$(LIBNAME)
	$(RM) -f $(INSTALL_INCLUDE)/oldplayerclient.h
	$(RM) -f $(INSTALL_INCLUDE)/olddevicedatatable.h

clean:
	$(RM) -f $(LIBNAME) *.o
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
