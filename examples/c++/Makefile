#
# $Id$
#

include ../../Makefile.common

CFLAGS = -g -Wall $(PLAYER_PLATFORM)
INCLUDES = -I../../client_libs/c++ -I../../client_libs/c -I../../include -I$(INSTALL_PREFIX)/include -I/usr/local/player/include
LIBDIRS = -L../../client_libs/c++ -L$(INSTALL_PREFIX)/lib -L/usr/local/player/lib
LIBS = $(LIBDIRS) -lplayerclient $(PLATFORM_LIBS) -lm

BINARIES = randomwalk \
	   visualservo \
           sonarobstacleavoid \
	   goto \
	   ptz \
	   laserobstacleavoid \
           example_from_manual \
	   say \
	   truth \
	   grip \
	   forage \
	   multilogger

all: $(BINARIES)

%: %.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS) -lpthread

install: $(BINARIES)
	@echo "Installing C++ example sources"
	$(MKDIR) -p $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 Makefile $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 randomwalk.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 visualservo.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 sonarobstacleavoid.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 goto.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 ptz.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 laserobstacleavoid.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 example_from_manual.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 say.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 truth.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 grip.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 forage.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 multilogger.cc $(INSTALL_EXAMPLES)/c++
	@echo "Installing C++ example binaries"
	$(INSTALL) -m 644 randomwalk $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 visualservo $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 sonarobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 goto $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 ptz $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 laserobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 example_from_manual $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 say $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 truth $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 grip $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 forage $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 multilogger $(INSTALL_EXAMPLES)/c++

uninstall: 
	@echo "Uninstalling example source files"
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/Makefile 
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/randomwalk.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/visualservo.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/sonarobstacleavoid.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/goto.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/ptz.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/laserobstacleavoid.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/example_from_manual.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/say.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/truth.cc 
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/grip.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/forage.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/multilogger.cc
	@echo "Uninstalling example binaries"
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/randomwalk
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/visualservo
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/sonarobstacleavoid
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/goto
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/ptz
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/laserobstacleavoid
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/example_from_manual
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/say
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/truth
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/grip
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/forage
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/multilogger

clean:
	$(RM) -f *.o $(BINARIES) test
