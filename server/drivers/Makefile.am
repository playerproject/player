SUBDIRS = p2os rwi stage isense reb


CXX = @CXX_SERVER@
AR = @AR_SERVER@
RANLIB = @RANLIB_SERVER@
CXXFLAGS = @CXXFLAGS_SERVER@

noinst_LIBRARIES = @ACTS_LIB@ @FESTIVAL_LIB@ \
		@LASER_LIB@ \
		@SICK_LIB@ @SONY_LIB@ @UDPBCAST_LIB@ \
		@FIXEDTONES_LIB@ @WIFI_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server


EXTRA_LIBRARIES = libfixedtones.a libwifi.a libacts.a libsicklms200.a \
	libfestival.a libsonyevid30.a libudpbroadcast.a \
	liblaserreflector.a liblaserbarcode.a liblaservisualbarcode.a liblasercspace.a

libsicklms200_a_SOURCES = sicklms200.cc
libfestival_a_SOURCES = festival.cc
libacts_a_SOURCES = acts.cc
libsonyevid30_a_SOURCES = sonyevid30.cc
libudpbroadcast_a_SOURCES = udpbroadcast.cc
liblaserbarcode_a_SOURCES = laserbarcode.cc
liblaserreflector_a_SOURCES = laserreflector.cc
liblaservisualbarcode_a_SOURCES = laservisualbarcode.cc
libfixedtones_a_SOURCES = fixedtones.cc
libwifi_a_SOURCES = wifi.cc
liblasercspace_a_SOURCES = lasercspace.cc




