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
	   truth \
	   grip \
	   forage \
	   multilogger stress autolayout 
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
	$(INSTALL) -m 644 truth.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 grip.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 forage.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 multilogger.cc $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 644 rsn.cc $(INSTALL_EXAMPLES)/c++
	@echo "Installing C++ example binaries"
	$(INSTALL) -m 755 randomwalk $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 visualservo $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 sonarobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 goto $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 ptz $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 laserobstacleavoid $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 example_from_manual $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 say $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 truth $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 grip $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 forage $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 multilogger $(INSTALL_EXAMPLES)/c++
	$(INSTALL) -m 755 rsn $(INSTALL_EXAMPLES)/c++

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
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/rsn.cc
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
	$(RM) -f  $(INSTALL_EXAMPLES)/c++/rsn

clean:
	$(RM) -f *.o $(BINARIES) test
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND

autolayout.o: ../../include/playermulticlient.h ../../include/playerclient.h
autolayout.o: ../../include/messages.h ../../include/playercommon.h
autolayout.o: ../../include/defaults.h ../../include/playercclient.h
autolayout.o: ../../include/laserproxy.h ../../include/clientproxy.h
autolayout.o: ../../include/sonarproxy.h ../../include/miscproxy.h
autolayout.o: ../../include/positionproxy.h ../../include/ptzproxy.h
autolayout.o: ../../include/gripperproxy.h ../../include/speechproxy.h
autolayout.o: ../../include/laserbeaconproxy.h ../../include/visionproxy.h
autolayout.o: ../../include/truthproxy.h ../../include/occupancyproxy.h
autolayout.o: ../../include/gpsproxy.h ../../include/bpsproxy.h
autolayout.o: ../../include/broadcastproxy.h ../../include/moteproxy.h
example_from_manual.o: ../../include/playerclient.h ../../include/messages.h
example_from_manual.o: ../../include/playercommon.h ../../include/defaults.h
example_from_manual.o: ../../include/playercclient.h
example_from_manual.o: ../../include/laserproxy.h ../../include/clientproxy.h
example_from_manual.o: ../../include/sonarproxy.h ../../include/miscproxy.h
example_from_manual.o: ../../include/positionproxy.h ../../include/ptzproxy.h
example_from_manual.o: ../../include/gripperproxy.h
example_from_manual.o: ../../include/speechproxy.h
example_from_manual.o: ../../include/laserbeaconproxy.h
example_from_manual.o: ../../include/visionproxy.h ../../include/truthproxy.h
example_from_manual.o: ../../include/occupancyproxy.h
example_from_manual.o: ../../include/gpsproxy.h ../../include/bpsproxy.h
example_from_manual.o: ../../include/broadcastproxy.h
example_from_manual.o: ../../include/moteproxy.h
forage.o: ../../include/playerclient.h ../../include/messages.h
forage.o: ../../include/playercommon.h ../../include/defaults.h
forage.o: ../../include/playercclient.h ../../include/laserproxy.h
forage.o: ../../include/clientproxy.h ../../include/sonarproxy.h
forage.o: ../../include/miscproxy.h ../../include/positionproxy.h
forage.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
forage.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
forage.o: ../../include/visionproxy.h ../../include/truthproxy.h
forage.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
forage.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
forage.o: ../../include/moteproxy.h
goto.o: ../../include/playerclient.h ../../include/messages.h
goto.o: ../../include/playercommon.h ../../include/defaults.h
goto.o: ../../include/playercclient.h ../../include/laserproxy.h
goto.o: ../../include/clientproxy.h ../../include/sonarproxy.h
goto.o: ../../include/miscproxy.h ../../include/positionproxy.h
goto.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
goto.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
goto.o: ../../include/visionproxy.h ../../include/truthproxy.h
goto.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
goto.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
goto.o: ../../include/moteproxy.h
grip.o: ../../include/playerclient.h ../../include/messages.h
grip.o: ../../include/playercommon.h ../../include/defaults.h
grip.o: ../../include/playercclient.h ../../include/laserproxy.h
grip.o: ../../include/clientproxy.h ../../include/sonarproxy.h
grip.o: ../../include/miscproxy.h ../../include/positionproxy.h
grip.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
grip.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
grip.o: ../../include/visionproxy.h ../../include/truthproxy.h
grip.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
grip.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
grip.o: ../../include/moteproxy.h
laserobstacleavoid.o: ../../include/playerclient.h ../../include/messages.h
laserobstacleavoid.o: ../../include/playercommon.h ../../include/defaults.h
laserobstacleavoid.o: ../../include/playercclient.h
laserobstacleavoid.o: ../../include/laserproxy.h ../../include/clientproxy.h
laserobstacleavoid.o: ../../include/sonarproxy.h ../../include/miscproxy.h
laserobstacleavoid.o: ../../include/positionproxy.h ../../include/ptzproxy.h
laserobstacleavoid.o: ../../include/gripperproxy.h
laserobstacleavoid.o: ../../include/speechproxy.h
laserobstacleavoid.o: ../../include/laserbeaconproxy.h
laserobstacleavoid.o: ../../include/visionproxy.h ../../include/truthproxy.h
laserobstacleavoid.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
laserobstacleavoid.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
laserobstacleavoid.o: ../../include/moteproxy.h
multilogger.o: ../../include/playermulticlient.h ../../include/playerclient.h
multilogger.o: ../../include/messages.h ../../include/playercommon.h
multilogger.o: ../../include/defaults.h ../../include/playercclient.h
multilogger.o: ../../include/laserproxy.h ../../include/clientproxy.h
multilogger.o: ../../include/sonarproxy.h ../../include/miscproxy.h
multilogger.o: ../../include/positionproxy.h ../../include/ptzproxy.h
multilogger.o: ../../include/gripperproxy.h ../../include/speechproxy.h
multilogger.o: ../../include/laserbeaconproxy.h ../../include/visionproxy.h
multilogger.o: ../../include/truthproxy.h ../../include/occupancyproxy.h
multilogger.o: ../../include/gpsproxy.h ../../include/bpsproxy.h
multilogger.o: ../../include/broadcastproxy.h ../../include/moteproxy.h
multimapping.o: ../../include/playermulticlient.h
multimapping.o: ../../include/playerclient.h ../../include/messages.h
multimapping.o: ../../include/playercommon.h ../../include/defaults.h
multimapping.o: ../../include/playercclient.h ../../include/laserproxy.h
multimapping.o: ../../include/clientproxy.h ../../include/sonarproxy.h
multimapping.o: ../../include/miscproxy.h ../../include/positionproxy.h
multimapping.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
multimapping.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
multimapping.o: ../../include/visionproxy.h ../../include/truthproxy.h
multimapping.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
multimapping.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
multimapping.o: ../../include/moteproxy.h
multirandom.o: ../../include/playermulticlient.h ../../include/playerclient.h
multirandom.o: ../../include/messages.h ../../include/playercommon.h
multirandom.o: ../../include/defaults.h ../../include/playercclient.h
multirandom.o: ../../include/laserproxy.h ../../include/clientproxy.h
multirandom.o: ../../include/sonarproxy.h ../../include/miscproxy.h
multirandom.o: ../../include/positionproxy.h ../../include/ptzproxy.h
multirandom.o: ../../include/gripperproxy.h ../../include/speechproxy.h
multirandom.o: ../../include/laserbeaconproxy.h ../../include/visionproxy.h
multirandom.o: ../../include/truthproxy.h ../../include/occupancyproxy.h
multirandom.o: ../../include/gpsproxy.h ../../include/bpsproxy.h
multirandom.o: ../../include/broadcastproxy.h ../../include/moteproxy.h
ptz.o: ../../include/playerclient.h ../../include/messages.h
ptz.o: ../../include/playercommon.h ../../include/defaults.h
ptz.o: ../../include/playercclient.h ../../include/laserproxy.h
ptz.o: ../../include/clientproxy.h ../../include/sonarproxy.h
ptz.o: ../../include/miscproxy.h ../../include/positionproxy.h
ptz.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
ptz.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
ptz.o: ../../include/visionproxy.h ../../include/truthproxy.h
ptz.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
ptz.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
ptz.o: ../../include/moteproxy.h
randomwalk.o: ../../include/playerclient.h ../../include/messages.h
randomwalk.o: ../../include/playercommon.h ../../include/defaults.h
randomwalk.o: ../../include/playercclient.h ../../include/laserproxy.h
randomwalk.o: ../../include/clientproxy.h ../../include/sonarproxy.h
randomwalk.o: ../../include/miscproxy.h ../../include/positionproxy.h
randomwalk.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
randomwalk.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
randomwalk.o: ../../include/visionproxy.h ../../include/truthproxy.h
randomwalk.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
randomwalk.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
randomwalk.o: ../../include/moteproxy.h
rsn.o: ../../include/playerclient.h ../../include/messages.h
rsn.o: ../../include/playercommon.h ../../include/defaults.h
rsn.o: ../../include/playercclient.h ../../include/laserproxy.h
rsn.o: ../../include/clientproxy.h ../../include/sonarproxy.h
rsn.o: ../../include/miscproxy.h ../../include/positionproxy.h
rsn.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
rsn.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
rsn.o: ../../include/visionproxy.h ../../include/truthproxy.h
rsn.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
rsn.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
rsn.o: ../../include/moteproxy.h
say.o: ../../include/playerclient.h ../../include/messages.h
say.o: ../../include/playercommon.h ../../include/defaults.h
say.o: ../../include/playercclient.h ../../include/laserproxy.h
say.o: ../../include/clientproxy.h ../../include/sonarproxy.h
say.o: ../../include/miscproxy.h ../../include/positionproxy.h
say.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
say.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
say.o: ../../include/visionproxy.h ../../include/truthproxy.h
say.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
say.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
say.o: ../../include/moteproxy.h
sonarobstacleavoid.o: ../../include/playerclient.h ../../include/messages.h
sonarobstacleavoid.o: ../../include/playercommon.h ../../include/defaults.h
sonarobstacleavoid.o: ../../include/playercclient.h
sonarobstacleavoid.o: ../../include/laserproxy.h ../../include/clientproxy.h
sonarobstacleavoid.o: ../../include/sonarproxy.h ../../include/miscproxy.h
sonarobstacleavoid.o: ../../include/positionproxy.h ../../include/ptzproxy.h
sonarobstacleavoid.o: ../../include/gripperproxy.h
sonarobstacleavoid.o: ../../include/speechproxy.h
sonarobstacleavoid.o: ../../include/laserbeaconproxy.h
sonarobstacleavoid.o: ../../include/visionproxy.h ../../include/truthproxy.h
sonarobstacleavoid.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
sonarobstacleavoid.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
sonarobstacleavoid.o: ../../include/moteproxy.h
stress.o: ../../include/playermulticlient.h ../../include/playerclient.h
stress.o: ../../include/messages.h ../../include/playercommon.h
stress.o: ../../include/defaults.h ../../include/playercclient.h
stress.o: ../../include/laserproxy.h ../../include/clientproxy.h
stress.o: ../../include/sonarproxy.h ../../include/miscproxy.h
stress.o: ../../include/positionproxy.h ../../include/ptzproxy.h
stress.o: ../../include/gripperproxy.h ../../include/speechproxy.h
stress.o: ../../include/laserbeaconproxy.h ../../include/visionproxy.h
stress.o: ../../include/truthproxy.h ../../include/occupancyproxy.h
stress.o: ../../include/gpsproxy.h ../../include/bpsproxy.h
stress.o: ../../include/broadcastproxy.h ../../include/moteproxy.h
truth.o: ../../include/playerclient.h ../../include/messages.h
truth.o: ../../include/playercommon.h ../../include/defaults.h
truth.o: ../../include/playercclient.h ../../include/laserproxy.h
truth.o: ../../include/clientproxy.h ../../include/sonarproxy.h
truth.o: ../../include/miscproxy.h ../../include/positionproxy.h
truth.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
truth.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
truth.o: ../../include/visionproxy.h ../../include/truthproxy.h
truth.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
truth.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
truth.o: ../../include/moteproxy.h
visualservo.o: ../../include/playerclient.h ../../include/messages.h
visualservo.o: ../../include/playercommon.h ../../include/defaults.h
visualservo.o: ../../include/playercclient.h ../../include/laserproxy.h
visualservo.o: ../../include/clientproxy.h ../../include/sonarproxy.h
visualservo.o: ../../include/miscproxy.h ../../include/positionproxy.h
visualservo.o: ../../include/ptzproxy.h ../../include/gripperproxy.h
visualservo.o: ../../include/speechproxy.h ../../include/laserbeaconproxy.h
visualservo.o: ../../include/visionproxy.h ../../include/truthproxy.h
visualservo.o: ../../include/occupancyproxy.h ../../include/gpsproxy.h
visualservo.o: ../../include/bpsproxy.h ../../include/broadcastproxy.h
visualservo.o: ../../include/moteproxy.h
