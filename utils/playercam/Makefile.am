AM_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/libplayerc @GTK_CFLAGS@

bin_PROGRAMS =

if WITH_GTK
bin_PROGRAMS += playercam
endif

playercam_SOURCES = playercam.c
playercam_LDADD = $(top_builddir)/client_libs/libplayerc/libplayerc.la $(top_builddir)/libplayerxdr/libplayerxdr.la @GTK_LIBS@
