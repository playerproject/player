SUBDIRS = v4l 1394 compress

noinst_LIBRARIES = @IMAGESEQ_LIB@

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/libplayerpacket

EXTRA_LIBRARIES = libimageseq.a

libimageseq_a_SOURCES = imageseq.cc

