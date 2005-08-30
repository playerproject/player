AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/client_libs -I$(top_srcdir) @GNOMECANVAS_CFLAGS@

if WITH_GNOMECANVAS
bin_PROGRAMS = playernav
endif

playernav_SOURCES =  playernav.c gui.c playernav.h player.c parse.c

playernav_LDADD = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la @GNOMECANVAS_LIBS@
