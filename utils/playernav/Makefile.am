if WITH_GTK
PLAYERNAV = playernav
endif

AM_CPPFLAGS = -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server @GTK_CFLAGS@

bin_PROGRAMS = $(PLAYERNAV)

all: playernav

playernav_SOURCES =  playernav.c

playernav_LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc @GTK_LIBS@
