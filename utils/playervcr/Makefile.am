AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/server @GTK_CFLAGS@

if WITH_GTK
bin_PROGRAMS = playervcr
endif

playervcr_SOURCES =  playervcr.c

playervcr_LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc @GTK_LIBS@
