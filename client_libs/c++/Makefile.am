SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/server -I$(srcdir)/../c -I$(srcdir)/../libplayerpacket -I$(top_srcdir)/replace

lib_LIBRARIES = libplayerclient.a
#lib_LTLIBRARIES = libplayerclient.la

libplayerclient_a_LIBADD = $(srcdir)/../c/playercclient.o $(top_builddir)/replace/*.o $(srcdir)/../libplayerpacket/playerpacket.o 
#libplayerclient_la_LIBADD = $(srcdir)/../c/playercclient.lo $(top_builddir)/replace/*.lo $(srcdir)/../libplayerpacket/playerpacket.lo 

libplayerclient_a_DEPENDENCIES = $(srcdir)/../c/playercclient.o  $(srcdir)/../libplayerpacket/playerpacket.o 
#libplayerclient_la_DEPENDENCIES = $(srcdir)/../c/playercclient.lo  $(srcdir)/../libplayerpacket/playerpacket.lo 

#libplayerclient_la_LDFLAGS =   -version-info 1:0:0

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
                              waveformproxy.cc \
                              mcomproxy.cc \
			      localizeproxy.cc \
			      soundproxy.cc \
                              audiodspproxy.cc \
                              audiomixerproxy.cc
                              #bpsproxy.cc
                              #bpsproxy.h

include_HEADERS = playerclient.h

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/cppclient.ps $(top_distdir)/doc
