SUBDIRS = p2os rwi stage reb isense microstrain

noinst_LIBRARIES = @ACTS_LIB@ @FESTIVAL_LIB@ \
		@LASER_LIB@ \
		@SICK_LIB@ @SONY_LIB@ @UDPBCAST_LIB@ \
		@FIXEDTONES_LIB@ @LINUXWIFI_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server


EXTRA_LIBRARIES = libfixedtones.a liblinuxwifi.a libacts.a libsicklms200.a \
	libfestival.a libsonyevid30.a libudpbroadcast.a \
	liblasercspace.a liblaserbar.a liblaserbarcode.a liblaservisualbarcode.a 

libsicklms200_a_SOURCES = sicklms200.cc
libfestival_a_SOURCES = festival.cc
libacts_a_SOURCES = acts.cc
libsonyevid30_a_SOURCES = sonyevid30.cc
libudpbroadcast_a_SOURCES = udpbroadcast.cc
liblasercspace_a_SOURCES = lasercspace.cc
liblaserbar_a_SOURCES = laserbar.cc
liblaserbarcode_a_SOURCES = laserbarcode.cc
liblaservisualbarcode_a_SOURCES = laservisualbarcode.cc
libfixedtones_a_SOURCES = fixedtones.cc
liblinuxwifi_a_SOURCES = linuxwifi.cc





