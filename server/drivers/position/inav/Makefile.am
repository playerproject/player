SUBDIRS = imap

if INCLUDE_RTKGUI
AM_CPPFLAGS = -I$(top_srcdir)/server -I@RTK_DIR@/include `gtk-config --cflags`
else
AM_CPPFLAGS = -I$(top_srcdir)/server 
endif

noinst_LIBRARIES = @INAV_LIB@

EXTRA_LIBRARIES = libinav.a

libinav_a_SOURCES = inav.cc inav_vector.c
libinav_a_LIBADD = imap/imap.o imap/imap_range.o imap/imap_fit.o imap/imap_store.o imap/imap_draw.o
