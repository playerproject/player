SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(srcdir)/../c

lib_LIBRARIES = libplayerclient.a

libplayerclient_a_LIBADD = ../c/playercclient.o
libplayerclient_a_DEPENDENCIES = ../c/playercclient.o

libplayerclient_a_SOURCES =  bpsproxy.cc \
                              bpsproxy.h \
                              broadcastproxy.cc \
                              broadcastproxy.h \
                              clientproxy.cc \
                              clientproxy.h \
                              descartesproxy.cc \
                              descartesproxy.h \
                              gpsproxy.cc \
                              gpsproxy.h \
                              gripperproxy.cc \
                              gripperproxy.h \
                              idarproxy.cc \
                              idarproxy.h \
                              idarturretproxy.cc \
                              idarturretproxy.h \
                              laserbeaconproxy.cc \
                              laserbeaconproxy.h \
                              laserproxy.cc \
                              laserproxy.h \
                              miscproxy.cc \
                              miscproxy.h \
                              moteproxy.cc \
                              moteproxy.h \
                              occupancyproxy.cc \
                              occupancyproxy.h \
                              playerclient.cc \
                              playerclient.h \
                              playermulticlient.cc \
                              playermulticlient.h \
                              positionproxy.cc \
                              positionproxy.h \
                              ptzproxy.cc \
                              ptzproxy.h \
                              rwi_bumperproxy.cc \
                              rwi_bumperproxy.h \
                              rwi_laserproxy.cc \
                              rwi_laserproxy.h \
                              rwi_positionproxy.cc \
                              rwi_positionproxy.h \
                              rwi_powerproxy.cc \
                              rwi_powerproxy.h \
                              rwi_sonarproxy.cc \
                              rwi_sonarproxy.h \
                              sonarproxy.cc \
                              sonarproxy.h \
                              speechproxy.cc \
                              speechproxy.h \
                              truthproxy.cc \
                              truthproxy.h \
                              visionproxy.cc \
                              visionproxy.h 

include_HEADERS = bpsproxy.h \
                  broadcastproxy.h \
                  clientproxy.h \
                  descartesproxy.h \
                  gpsproxy.h \
                  gripperproxy.h \
                  idarproxy.h \
                  idarturretproxy.h \
                  laserbeaconproxy.h \
                  laserproxy.h \
                  miscproxy.h \
                  moteproxy.h \
                  occupancyproxy.h \
                  playerclient.h \
                  playermulticlient.h \
                  positionproxy.h \
                  ptzproxy.h \
                  rwi_bumperproxy.h \
                  rwi_laserproxy.h \
                  rwi_positionproxy.h \
                  rwi_powerproxy.h \
                  rwi_sonarproxy.h \
                  sonarproxy.h \
                  speechproxy.h \
                  truthproxy.h \
                  visionproxy.h

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/cppclient.ps $(top_distdir)/doc
