SUBDIRS = p2os rwi stage

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = libacts.a libfestival.a liblaserbarcode.a libsicklms200.a libsonyevid30.a libudpbroadcast.a @FIXEDTONES_LIB@

EXTRA_LIBRARIES = libfixedtones.a

libsicklms200_a_SOURCES = sicklms200.cc
libfestival_a_SOURCES = festival.cc
libacts_a_SOURCES = acts.cc
libsonyevid30_a_SOURCES = sonyevid30.cc
libudpbroadcast_a_SOURCES = udpbroadcast.cc
liblaserbarcode_a_SOURCES = laserbarcode.cc
libfixedtones_a_SOURCES = fixedtones.cc
