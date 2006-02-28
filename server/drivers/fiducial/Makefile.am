noinst_LIBRARIES = @LASERBAR_LIB@ @LASERBARCODE_LIB@ @LASERVISUALBARCODE_LIB@ @LASERVISUALBW_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

EXTRA_DIST = beacon.jpg laservisualbeacon.jpg

EXTRA_LIBRARIES = liblaserbar.a liblaserbarcode.a liblaservisualbarcode.a liblaservisualbw.a

liblaserbar_a_SOURCES = laserbar.cc
liblaserbarcode_a_SOURCES = laserbarcode.cc
liblaservisualbarcode_a_SOURCES = laservisualbarcode.cc
liblaservisualbw_a_SOURCES = laservisualbw.cc
