
SUBDIRS = cmvision upcbarcode

noinst_LIBRARIES = @ACTS_LIB@ @SIMPLESHAPE_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

EXTRA_LIBRARIES = libacts.a libsimpleshape.a

libacts_a_SOURCES = acts.cc
libsimpleshape_a_SOURCES = simpleshape.cc
