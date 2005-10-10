bin_PROGRAMS = playerprint

playerprint_CFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/libplayerc++ -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/client_libs
playerprint_LDADD = $(top_builddir)/client_libs/libplayerc/.libs/libplayerc.a $(top_builddir)/client_libs/libplayerc/.libs/libplayerc.a $(top_builddir)/libplayerxdr/.libs/libplayerxdr.a $(top_builddir)/libplayercore/.libs/libplayercore.a 

playerprint_SOURCES = playerprint.c
