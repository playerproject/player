SUBDIRS = .

lib_LTLIBRARIES =

# we need Boost, and so does our test program
if INCLUDE_BOOST_SIGNALS
 lib_LTLIBRARIES += libplayerc++.la
 SUBDIRS += test
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
                              cameraproxy.cc \
                              laserproxy.cc

                              #gpsproxy.cc \
                              actarrayproxy.cc \
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

libplayerc___la_LIBADD = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lpthread
libplayerc___la_DEPENDENCIES = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la

libplayerc___includedir = $(includedir)/libplayerc++
libplayerc___include_HEADERS = playerc++.h
