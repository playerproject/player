if WITH_SICKLMS200
BUILDSICKLMS200 = libsicklms200.a
else
BUILDSICKLMS200 =
endif
if WITH_FESTIVAL
BUILDFESTIVAL = libfestival.a
else
BUILDFESTIVAL =
endif
if WITH_ACTS
BUILDACTS = libacts.a
else
BUILDACTS =
endif
if WITH_SONYEVID30
BUILDSONYEVID30 = libsonyevid30.a
else
BUILDSONYEVID30 =
endif
if WITH_UDPBROADCAST
BUILDUDPBROADCAST = libudpbroadcast.a
else
BUILDUDPBROADCAST =
endif
if WITH_LASERBARCODE
BUILDLASERBARCODE = liblaserbarcode.a
else
BUILDLASERBARCODE =
endif

SUBDIRS = p2os rwi stage

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = $(BUILDSICKLMS200) $(BUILDFESTIVAL) $(BUILDACTS) \
                   $(BUILDSONYEVID30) $(BUILDUDPBROADCAST) \
		   $(BUILDLASERBARCODE)

libsicklms200_a_SOURCES = sicklms200.cc
libfestival_a_SOURCES = festival.cc
libacts_a_SOURCES = acts.cc
libsonyevid30_a_SOURCES = sonyevid30.cc
libudpbroadcast_a_SOURCES = udpbroadcast.cc
liblaserbarcode_a_SOURCES = laserbarcode.cc

clean-local:
	rm -f *.a
