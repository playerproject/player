#
# $Id$
#

CPP = g++
RM = rm
INSTALL = install
MKDIR = mkdir
RMDIR = rmdir

INSTALL_EXAMPLES = $(INSTALL_PREFIX)/examples/c++

CFLAGS = -g -Wall 
INCLUDES = -I../../client_libs/c++ -I../../client_libs/c -I../../include -I$(INSTALL_PREFIX)/include -I/usr/local/player/include
LIBDIRS = -L../../client_libs/c++ -L$(INSTALL_PREFIX)/lib -L/usr/local/player/lib
LIBS = $(LIBDIRS) -lplayerclient

BINARIES = randomwalk \
           sonarobstacleavoid \
	   goto \
	   ptz \
	   laserobstacleavoid \
           example_from_manual

all: $(BINARIES)

example_from_manual: example_from_manual.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

randomwalk: randomwalk.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

sonarobstacleavoid: sonarobstacleavoid.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

laserobstacleavoid: laserobstacleavoid.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

goto: goto.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS) -lpthread

ptz: ptz.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

test: test.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

install: $(BINARIES)
	$(MKDIR) -p $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 Makefile $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 example_from_manual.cc $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 randomwalk.cc $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 sonarobstacleavoid.cc $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 laserobstacleavoid.cc $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 goto.cc $(INSTALL_EXAMPLES)
	$(INSTALL) -m 644 ptz.cc $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 example_from_manual $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 randomwalk $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 sonarobstacleavoid $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 laserobstacleavoid $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 goto $(INSTALL_EXAMPLES)
	$(INSTALL) -m 755 ptz $(INSTALL_EXAMPLES)

uninstall: 
	$(RM) -f $(INSTALL_EXAMPLES)/Makefile 
	$(RM) -f $(INSTALL_EXAMPLES)/example_from_manual.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/randomwalk.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/sonarobstacleavoid.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/laserobstacleavoid.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/goto.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/ptz.cc 
	$(RM) -f $(INSTALL_EXAMPLES)/example_from_manual 
	$(RM) -f $(INSTALL_EXAMPLES)/randomwalk 
	$(RM) -f $(INSTALL_EXAMPLES)/sonarobstacleavoid 
	$(RM) -f $(INSTALL_EXAMPLES)/laserobstacleavoid
	$(RM) -f $(INSTALL_EXAMPLES)/goto 
	$(RM) -f $(INSTALL_EXAMPLES)/ptz 

clean:
	$(RM) -f *.o $(BINARIES) test
