SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/server -I$(srcdir)/../c -I$(top_srcdir)/replace

lib_LIBRARIES = libplayerclient.a

libplayerclient_a_LIBADD = $(srcdir)/../c/playercclient.o $(top_builddir)/replace/libreplace.a
libplayerclient_a_DEPENDENCIES = $(srcdir)/../c/playercclient.o $(top_builddir)/replace/libreplace.a

libplayerclient_a_SOURCES =   commsproxy.cc \
                              clientproxy.cc \
                              descartesproxy.cc \
                              gpsproxy.cc \
                              gripperproxy.cc \
                              idarproxy.cc \
                              idarturretproxy.cc \
                              fiducialproxy.cc \
                              laserproxy.cc \
                              moteproxy.cc \
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
                              waveformproxy.cc
                              #bpsproxy.cc
                              #bpsproxy.h

include_HEADERS = playerclient.h

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/cppclient.ps $(top_distdir)/doc
