SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir) -I$(srcdir)/../c -I$(srcdir)/../libplayerpacket 

# create the pkg-config entry for the C++
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerclient.pc

lib_LIBRARIES = libplayerclient.a

libplayerclient_a_LIBADD = $(top_builddir)/client_libs/c/playercclient.$(OBJEXT) $(top_builddir)/replace/*.$(OBJEXT) $(top_builddir)/client_libs/libplayerpacket/*.$(OBJEXT) 

libplayerclient_a_DEPENDENCIES = $(top_builddir)/client_libs/c/playercclient.$(OBJEXT) $(top_builddir)/client_libs/libplayerpacket/*.$(OBJEXT)


libplayerclient_a_SOURCES =   clientproxy.cc \
                              actarrayproxy.cc \
                              gpsproxy.cc \
                              gripperproxy.cc \
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
                              plannerproxy.cc \
                              mapproxy.cc \
                              energyproxy.cc \
                              speechrecognitionproxy.cc \
                              simulationproxy.cc \
                              logproxy.cc
                              #bpsproxy.cc
                              #bpsproxy.h

include_HEADERS = playerclient.h playermcomtypes.h
