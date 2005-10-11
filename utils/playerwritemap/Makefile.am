AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/client_libs -I$(top_srcdir) @GDK_PIXBUF_CFLAGS@ -fno-strict-aliasing

if WITH_GDKPIXBUF
bin_PROGRAMS = playerwritemap
endif

playerwritemap_SOURCES =  playerwritemap.c

playerwritemap_LDADD = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la @GDK_PIXBUF_LIBS@
