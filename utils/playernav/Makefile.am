if WITH_GNOMECANVAS
PLAYERNAV = playernav
endif

AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server @GNOMECANVAS_CFLAGS@

bin_PROGRAMS = $(PLAYERNAV)

all: playernav

playernav_SOURCES =  playernav.c

playernav_LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc @GNOMECANVAS_LIBS@
