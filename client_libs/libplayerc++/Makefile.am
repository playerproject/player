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
                              playerc++.cc \
                              playerclient.cc \
                              playererror.cc \
                              clientproxy.cc \
                              actarrayproxy.cc \
                              blobfinderproxy.cc \
                              bumperproxy.cc \
                              cameraproxy.cc \
                              fiducialproxy.cc \
                              gripperproxy.cc \
                              irproxy.cc \
                              laserproxy.cc \
                              localizeproxy.cc \
                              logproxy.cc \
                              mapproxy.cc \
                              plannerproxy.cc \
                              position2dproxy.cc \
                              position3dproxy.cc \
                              powerproxy.cc \
                              ptzproxy.cc

# not in libplayerc yet \
                              aioproxy.cc \
                              audioproxy.cc \
                              audiodspproxy.cc \
                              audiomixerproxy.cc \
                              blinkenlightproxy.cc \
                              dioproxy.cc \
                              energyproxy.cc \
                              gpsproxy.cc \
                              mcomproxy.cc \
                              motorproxy.cc

# not looked at yet \
                              sonarproxy.cc \
                              speechproxy.cc \
                              truthproxy.cc \
                              wifiproxy.cc \
                              waveformproxy.cc \
                              soundproxy.cc \
                              speechrecognitionproxy.cc \
                              simulationproxy.cc

libplayerc___la_LIBADD = -L$(top_builddir)/libplayerxdr -lplayerxdr -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm -l@BOOST_SIGNALS_LIB@ -l@BOOST_THREAD_LIB@ -lpthread
libplayerc___la_DEPENDENCIES = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la

libplayerc___includedir = $(includedir)/libplayerc++
libplayerc___include_HEADERS = playerc++.h playerclient.h playererror.h
