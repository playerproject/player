SUBDIRS = drivers

AM_CPPFLAGS = -fPIC -Wall @GAZEBO_EXTRA_CPPFLAGS@ -I$(srcdir) -I$(top_srcdir) -I$(srcdir)/drivers/gazebo -I$(srcdir)/drivers/shell

if INCLUDE_RTKGUI
RTKGUI_LIBS=-L$(top_builddir)/rtk2 -lrtk @GTK_LIBS@
else
RTKGUI_LIBS=
endif

player_DEPENDENCIES = @PLAYER_DRIVER_LIBPATHS@ $(top_builddir)/libplayercore/libplayercore.a

#
# Note to everybody: please do NOT break the following assignment onto
# separate lines with backslashes.  It makes the link line look like shit and 
# hard to parse.  - BPG
#
player_LDADD   = -lm @PLAYER_DRIVER_LIBPATHS@ -L$(top_builddir)/libplayercore -lplayercore -L$(top_builddir)/client_libs/libplayerpacket -lplayerpacket @PLAYER_DRIVER_EXTRA_LIBS@ $(RTKGUI_LIBS) -L$(top_builddir)/replace -lreplace

player_LDFLAGS = -export-dynamic

bin_PROGRAMS = player

player_SOURCES = clientdata.cc socket_util.cc \
		 clientmanager.cc main.cc \
                 clientdata.h clientmanager.h socket_util.h \
		 timer.cc timer.h \
                 driverregistry.cc driverregistry.h
