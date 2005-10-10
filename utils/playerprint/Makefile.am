bin_PROGRAMS = playerprint

playerprint_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs
playerprint_LDADD = -L$(top_srcdir)/client_libs/libplayerc/
playerprint_LDADD += -L$(top_srcdir)/client_libs/libplayerc++/
playerprint_LDADD += -L$(top_srcdir)/libplayercore/
playerprint_LDADD += -lplayerc -lplayerc++ -lplayererror -lplayerxdr -lplayercore -lboost_signals
playerprint_SOURCES = playerprint.cc
