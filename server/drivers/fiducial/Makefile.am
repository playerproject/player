noinst_LIBRARIES = @LASERBAR_LIB@ @LASERBARCODE_LIB@ @LASERVISUALBARCODE_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

EXTRA_LIBRARIES = liblaserbar.a liblaserbarcode.a \
                  liblaservisualbarcode.a 

liblaserbar_a_SOURCES = laserbar.cc
liblaserbarcode_a_SOURCES = laserbarcode.cc
liblaservisualbarcode_a_SOURCES = laservisualbarcode.cc
