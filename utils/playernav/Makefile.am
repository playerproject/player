AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server @GNOMECANVAS_CFLAGS@

if WITH_GNOMECANVAS
bin_PROGRAMS = playernav
endif

playernav_SOURCES =  playernav.c gui.c playernav.h player.c parse.c

playernav_LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc @GNOMECANVAS_LIBS@
