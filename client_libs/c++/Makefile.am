SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/server -I$(srcdir)/../c -I$(srcdir)/../libplayerpacket -I$(top_srcdir)/replace

lib_LIBRARIES = libplayerclient.a

libplayerclient_a_LIBADD = $(top_builddir)/client_libs/c/playercclient.o $(top_builddir)/replace/*.o $(top_builddir)/client_libs/libplayerpacket/playerpacket.o 

libplayerclient_a_DEPENDENCIES = $(top_builddir)/client_libs/c/playercclient.o  $(top_builddir)/client_libs/libplayerpacket/playerpacket.o 


libplayerclient_a_SOURCES =   commsproxy.cc \
                              clientproxy.cc \
                              descartesproxy.cc \
                              gpsproxy.cc \
                              gripperproxy.cc \
                              idarproxy.cc \
                              idarturretproxy.cc \
                              fiducialproxy.cc \
                              laserproxy.cc \
                              playerclient.cc \
                              playerclient.h \
                              playermulticlient.cc \
                              positionproxy.cc \
                              ptzproxy.cc \
                              sonarproxy.cc \
                              speechproxy.cc \
                              truthproxy.cc \
                              blobfinderproxy.cc \
                              wifiproxy.cc \
                              irproxy.cc \
                              powerproxy.cc \
                              bumperproxy.cc \
                              dioproxy.cc \
                              audioproxy.cc \
                              waveformproxy.cc \
                              mcomproxy.cc \
                              localizeproxy.cc \
                              soundproxy.cc \
                              audiodspproxy.cc \
                              audiomixerproxy.cc \
                              motorproxy.cc \
                              position2dproxy.cc \
                              position3dproxy.cc \
                              blinkenlightproxy.cc \
                              cameraproxy.cc \
                              aioproxy.cc \
                              hudproxy.cc \
                              plannerproxy.cc \
                              mapproxy.cc \
                              energyproxy.cc
                              #bpsproxy.cc
                              #bpsproxy.h

include_HEADERS = playerclient.h playermcomtypes.h

# REMOVE; docs are no longer built in the distribution
#dist-hook:
#	mkdir -p $(top_distdir)/doc
#	cd $(srcdir)/doc && make
#	cp -p $(srcdir)/doc/cppclient.ps $(top_distdir)/doc
