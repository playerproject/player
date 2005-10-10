bin_PROGRAMS = playerprint

playerprint_CFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/libplayerc++ -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/client_libs
playerprint_LDADD = -lplayerc -lplayerc++ -lplayererror -lplayerxdr -lplayercore -lboost_signals
playerprint_SOURCES = playerprint.cc
