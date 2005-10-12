bin_PROGRAMS =
if INCLUDE_PLAYERCC
 bin_PROGRAMS += playerprint
endif

playerprint_CPPFLAGS = -I$(top_srcdir)/libplayercore -I$(top_srcdir)/client_libs $(PLAYERCC_EXTRA_CFLAGS)
playerprint_LDADD = -L$(top_srcdir)/client_libs/libplayerc/
playerprint_LDADD += -L$(top_srcdir)/client_libs/libplayerc++/
playerprint_LDADD += -L$(top_srcdir)/libplayercore/
playerprint_LDADD += $(PLAYERCC_LIB)

playerprint_SOURCES = playerprint.cc
