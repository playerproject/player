#
# $Id$
#

include ../../Makefile.common

CFLAGS = -g -Wall $(PLAYER_PLATFORM)
INCLUDES = -I../../client_libs/c++ -I../../client_libs/c -I../../include -I$(INSTALL_PREFIX)/include -I/usr/local/player/include
LIBDIRS = -L../../client_libs/c++ -L$(INSTALL_PREFIX)/lib -L/usr/local/player/lib
LIBS = $(LIBDIRS) -lplayerclient $(PLATFORM_LIBS)

BINARIES = randomwalk \
           sonarobstacleavoid \
	   goto \
	   ptz \
	   laserobstacleavoid \
           example_from_manual

all: $(BINARIES)

example_from_manual: example_from_manual.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ example_from_manual.cc $(LIBS)

randomwalk: randomwalk.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ randomwalk.cc $(LIBS)

sonarobstacleavoid: sonarobstacleavoid.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ sonarobstacleavoid.cc $(LIBS)

laserobstacleavoid: laserobstacleavoid.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ laserobstacleavoid.cc $(LIBS)

goto: goto.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ goto.cc $(LIBS) -lpthread

ptz: ptz.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ ptz.cc $(LIBS)

test: test.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ test.cc $(LIBS)

install: $(BINARIES)
	$(MKDIR) -p $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 Makefile $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 example_from_manual.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 randomwalk.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 sonarobstacleavoid.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 laserobstacleavoid.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 goto.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 ptz.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 example_from_manual $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 randomwalk $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 sonarobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 laserobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 goto $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 ptz $(INSTALL_EXAMPLES)/c++

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/Makefile 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/example_from_manual.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/randomwalk.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/sonarobstacleavoid.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/laserobstacleavoid.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/goto.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/ptz.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/example_from_manual 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/randomwalk 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/sonarobstacleavoid 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/laserobstacleavoid
	$(RM) -f $(INSTALL_EXAMPLES)/c++/goto 
	$(RM) -f $(INSTALL_EXAMPLES)/c++/ptz 

clean:
	$(RM) -f *.o $(BINARIES) test
