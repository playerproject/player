SUBDIRS = . test doc

AM_CPPFLAGS = -Wall -g -I$(top_srcdir) -I$(srcdir)/../libplayerc

# create the pkg-config entry for the C++
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerc++.pc

EXTRA_DIST = playerc++.pc.in
CLEANFILES = playerc++.pc

lib_LTLIBRARIES = 
if INCLUDE_BOOST_SIGNALS
lib_LTLIBRARIES += libplayerc++.la
endif

# the +'s are turned into _'s by automake
libplayerc___la_LDFLAGS = -version-info 2:0:0

libplayerc___la_LIBADD = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lpthread
libplayerc___la_DEPENDENCIES = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la

libplayerc___la_SOURCES =   playercc.h \
                              playerclient.cc \
                              playererror.cc \
                              clientproxy.cc \
                              cameraproxy.cc

                              #gpsproxy.cc \
                              actarrayproxy.cc \
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

include_HEADERS = playercc.h
>>>>>>> 1.4
