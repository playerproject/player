SUBDIRS = drivers

AM_CPPFLAGS = -fPIC -Wall -g @GAZEBO_EXTRA_CPPFLAGS@ -I$(srcdir) -I$(top_srcdir) -I$(srcdir)/drivers/gazebo -I$(srcdir)/drivers/shell

if INCLUDE_RTKGUI
RTKGUI_LIBS=-L$(top_builddir)/rtk2 -lrtk @GTK_LIBS@
else
RTKGUI_LIBS=
endif

player_DEPENDENCIES = @PLAYER_DRIVER_LIBPATHS@ $(top_builddir)/libplayercore/libplayercore.a $(top_builddir)/libplayercore/libplayererror.a $(top_builddir)/libplayertcp/libplayertcp.a $(top_builddir)/libplayerxdr/libplayerxdr.a

# Note to everybody: please do NOT break the following assignment onto
# separate lines with backslashes.  It makes the link line look like shit and 
# hard to parse.  - BPG
#
player_LDADD   = -lm @PLAYER_DRIVER_LIBPATHS@ -L$(top_builddir)/libplayercore -lplayercore -lplayererror -L$(top_builddir)/libplayertcp -lplayertcp -L$(top_builddir)/libplayerxdr -lplayerxdr @PLAYER_DRIVER_EXTRA_LIBS@ $(RTKGUI_LIBS) -L$(top_builddir)/replace -lreplace

player_LDFLAGS = -export-dynamic

bin_PROGRAMS = player

player_SOURCES = server.cc driverregistry.cc driverregistry.h

