bin_PROGRAMS = 
if INCLUDE_BOOST_SIGNALS
bin_PROGRAMS+=playerjoy
endif

playerjoy_SOURCES = playerjoy.cc
playerjoy_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/libplayerc++ -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/client_libs
playerjoy_LDADD = -L$(top_srcdir)/client_libs/libplayerc
playerjoy_LDADD += -L$(top_srcdir)/client_libs/libplayerc++
playerjoy_LDADD += -L$(top_srcdir)/libplayercore
playerjoy_LDADD += -lplayerc -lplayerc++ -lplayererror -lplayerxdr -lplayercore -lboost_signals

