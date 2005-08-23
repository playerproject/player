AM_CPPFLAGS = -Wall -g -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir) @GTK_CFLAGS@

if WITH_GTK
bin_PROGRAMS = playervcr
endif

playervcr_SOURCES =  playervcr.c

playervcr_LDADD = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la @GTK_LIBS@
