SUBDIRS = imap

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = @INAV_LIB@

EXTRA_LIBRARIES = libinav.a

libinav_a_SOURCES = inav.cc inav_vector.c
libinav_a_LIBADD = imap/imap.o imap/imap_range.o imap/imap_fit.o
