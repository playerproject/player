#
# $Id$
#

include ../../Makefile.common

INCLUDES = -I../../include -I$(INSTALL_PREFIX)/include \
           -I/usr/local/player/include
LIBDIRS = -L../../lib -L$(INSTALL_PREFIX)/lib \
          -L/usr/local/player/lib
LIBS = $(LIBDIRS) -lplayerclient $(PLATFORM_LIBS) -lm

BINARIES = rsn \
           randomwalk \
	   visualservo \
           sonarobstacleavoid \
	   goto \
	   ptz \
	   laserobstacleavoid \
           example_from_manual \
	   say \
	   grip \
	   forage \
	   multilogger \
           stress \
           pherobot\
	  joystick
	   #multirandom multiphero

all: $(BINARIES)

%: %.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS) -lpthread

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

clean_dep:
	$(MAKEDEP) -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND"
	$(RM) -f Makefile.bak

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
	$(INSTALL) -m 644 grip.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 forage.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 multilogger.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 rsn.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 joystick.cc $(INSTALL_EXAMPLES)/c++
	@echo "Installing C++ example binaries"
	$(INSTALL) -m 755 randomwalk $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 visualservo $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 sonarobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 goto $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 ptz $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 laserobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 example_from_manual $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 say $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 grip $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 forage $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 multilogger $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 rsn $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 joystick $(INSTALL_EXAMPLES)/c++

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
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/grip.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/forage.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/multilogger.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/rsn.cc
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/joystick.cc
	@echo "Uninstalling example binaries"
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/randomwalk
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/visualservo
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/sonarobstacleavoid
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/goto
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/ptz
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/laserobstacleavoid
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/example_from_manual
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/say
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/grip
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/forage
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/multilogger
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/rsn
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/joystick

clean:
	$(RM) -f *.o $(BINARIES) test
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
