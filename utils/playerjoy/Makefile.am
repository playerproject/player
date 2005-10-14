
bin_PROGRAMS =
if INCLUDE_PLAYERCC
 bin_PROGRAMS += playerjoy
endif

playerjoy_SOURCES = playerjoy.cc
playerjoy_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs/libplayerc++ -I$(top_srcdir)/client_libs/libplayerc -I$(top_srcdir)/client_libs $(PLAYERCC_EXTRA_CFLAGS)
playerjoy_LDADD = $(PLAYERCC_LIB)

