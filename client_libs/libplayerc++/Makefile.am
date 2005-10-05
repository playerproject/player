SUBDIRS = . test

lib_LTLIBRARIES =

# we need Boost
if INCLUDE_BOOST_SIGNALS
 lib_LTLIBRARIES += libplayerc++.la
endif

AM_CPPFLAGS = -Wall -g -I$(top_srcdir) -I$(top_srcdir)/client_libs

# create the pkg-config entry for the C++
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerc++.pc

EXTRA_DIST = playerc++.pc.in
CLEANFILES = playerc++.pc

# the +'s are turned into _'s by automake
libplayerc___la_LDFLAGS = -version-info 2:0:0  -rpath $(libdir)

libplayerc___la_SOURCES =     playerc++.h \
                              playerclient.h \
                              playererror.h \
                              playerclient.cc \
                              playererror.cc \
                              clientproxy.cc \
                              actarrayproxy.cc \
                              blobfinderproxy.cc \
                              bumperproxy.cc \
                              cameraproxy.cc \
                              laserproxy.cc

# not in libplayerc yet \
                              aioproxy.cc \
                              audioproxy.cc \
                              audiodspproxy.cc \
                              audiomixerproxy.cc \
                              blinkenlightproxy.cc \
                              dioproxy.cc \
                              energyproxy.cc 

                              #gpsproxy.cc \
                              gripperproxy.cc \
                              fiducialproxy.cc \
                              playerclient.cc \
                              playerclient.h \
                              playermulticlient.cc \
                              positionproxy.cc \
                              ptzproxy.cc \
                              sonarproxy.cc \
                              speechproxy.cc \
                              truthproxy.cc \
                              wifiproxy.cc \
                              irproxy.cc \
                              powerproxy.cc \
                              waveformproxy.cc \
                              mcomproxy.cc \
                              localizeproxy.cc \
                              soundproxy.cc \
                              motorproxy.cc \
                              position2dproxy.cc \
                              position3dproxy.cc \
                              cameraproxy.cc \
                              plannerproxy.cc \
                              mapproxy.cc \
                              speechrecognitionproxy.cc \
                              simulationproxy.cc \
                              logproxy.cc
                              #bpsproxy.cc
                              #bpsproxy.h

libplayerc___la_LIBADD = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lpthread
libplayerc___la_DEPENDENCIES = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la

libplayerc___includedir = $(includedir)/libplayerc++
libplayerc___include_HEADERS = playerc++.h playerclient.h playererror.h
