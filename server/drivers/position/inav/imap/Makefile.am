

noinst_LIBRARIES = @INAV_IMAP_LIB@

EXTRA_LIBRARIES = libimap.a

if INCLUDE_RTKGUI
AM_CPPFLAGS = -I$(top_srcdir)/server -I@RTK_DIR@/include `gtk-config --cflags`
libimap_a_SOURCES = imap.h imap.c imap_range.c imap_fit.c imap_store.c imap_draw.c
else
AM_CPPFLAGS = -I$(top_srcdir)/server
libimap_a_SOURCES = imap.h imap.c imap_range.c imap_fit.c imap_store.c 
endif


