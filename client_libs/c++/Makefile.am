SUBDIRS = . test

AM_CPPFLAGS = -I$(top_srcdir)/server -I$(srcdir)/../c

lib_LIBRARIES = libplayerclient.a

libplayerclient_a_LIBADD = ../c/playercclient.o
libplayerclient_a_DEPENDENCIES = ../c/playercclient.o

libplayerclient_a_SOURCES =   commsproxy.cc \
                              clientproxy.cc \
                              descartesproxy.cc \
                              gpsproxy.cc \
                              gripperproxy.cc \
                              idarproxy.cc \
                              idarturretproxy.cc \
                              fiducialproxy.cc \
                              srfproxy.cc \
                              moteproxy.cc \
                              playerclient.cc \
                              playerclient.h \
                              playermulticlient.cc \
                              positionproxy.cc \
                              ptzproxy.cc \
                              frfproxy.cc \
                              speechproxy.cc \
                              truthproxy.cc \
                              blobfinderproxy.cc
                              #rwi_bumperproxy.cc \
                              #rwi_bumperproxy.h \
                              #rwi_laserproxy.cc \
                              #rwi_laserproxy.h \
                              #rwi_positionproxy.cc \
                              #rwi_positionproxy.h \
                              #rwi_powerproxy.cc \
                              #rwi_powerproxy.h \
                              #rwi_sonarproxy.cc \
                              #rwi_sonarproxy.h
                              #bpsproxy.cc
                              #bpsproxy.h

include_HEADERS = playerclient.h
                  

dist-hook:
	mkdir -p $(top_distdir)/doc
	cd $(srcdir)/doc && make manual
	cp -p $(srcdir)/doc/cppclient.ps $(top_distdir)/doc
