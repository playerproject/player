bin_PROGRAMS = 
if INCLUDE_BOOST_SIGNALS
bin_PROGRAMS+=playerprint
endif

playerprint_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs
playerprint_LDADD = -L$(top_srcdir)/client_libs/libplayerc/
playerprint_LDADD += -L$(top_srcdir)/client_libs/libplayerc++/
playerprint_LDADD += -L$(top_srcdir)/libplayercore/
playerprint_LDADD += -lplayerc -lplayerc++ -lplayererror -lplayerxdr -lplayercore
playerprint_SOURCES = playerprint.cc
