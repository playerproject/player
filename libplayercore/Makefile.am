AM_CPPFLAGS = -g -fPIC -Wall -D PLAYER_INSTALL_PREFIX="\"@PREFIX@\"" -I$(top_srcdir)

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playercore.pc playererror.pc

dist_bin_SCRIPTS = playerinterfacegen.py

EXTRA_DIST = playercore.pc.in playererror.pc.in interfaces/ADDING_INTERFACES interfaces/*.def

player_interfaces.h: ./interfaces/* playerinterfacegen.py
	if ! ./playerinterfacegen.py > player_interfaces.h; then rm player_interfaces.h; fi

interface_table.c: ./interfaces/* playerinterfacegen.py
	if ! ./playerinterfacegen.py --utils > interface_table.c; then rm interface_table.c; fi


BUILT_SOURCES = playerconfig.h player_interfaces.h interface_table.c
DISTCLEANFILES = playerconfig.h playercore.pc playererror.pc

lib_LTLIBRARIES = libplayererror.la libplayerutils.la libplayercore.la

libplayercore_la_SOURCES = driver.cc driver.h \
                          device.cc device.h \
                          drivertable.h drivertable.cc \
                          devicetable.h devicetable.cc \
                          configfile.cc configfile.h \
                          playercommon.h player.h playerconfig.h.in \
                          message.h message.cc \
                          wallclocktime.cc wallclocktime.h playertime.h \
                          plugins.cc plugins.h \
                          globals.cc globals.h \
                          addr_util.c addr_util.h \
                          property.cpp property.h
libplayercore_la_LDFLAGS = $(PLAYER_VERSION_INFO) -rpath $(libdir) $(top_builddir)/libplayercore/libplayererror.la $(top_builddir)/libplayercore/libplayerutils.la
libplayercore_la_DEPENDENCIES = $(top_builddir)/libplayercore/libplayererror.la $(top_builddir)/libplayercore/libplayerutils.la player_interfaces.h

libplayererror_la_SOURCES = error.h error.c
libplayererror_la_LDFLAGS = $(PLAYER_VERSION_INFO) -rpath $(libdir)

libplayerutils_la_DEPENDENCIES = interface_table.c
libplayerutils_la_SOURCES = interface_util.c interface_util.h 
libplayerutils_la_LDFLAGS = $(PLAYER_VERSION_INFO) -rpath $(libdir)



coreincludedir = $(includedir)/player-2.0/libplayercore
coreinclude_HEADERS = configfile.h \
                      device.h \
                      devicetable.h \
                      driver.h \
                      drivertable.h \
                      globals.h \
                      interface_util.h \
                      message.h \
                      player.h \
                      playercommon.h \
                      playerconfig.h \
                      playercore.h \
                      playertime.h \
                      plugins.h \
                      wallclocktime.h \
                      error.h \
                      addr_util.h \
                      property.h \
                      player_interfaces.h
                      
player_interfaces.h: ./interfaces/* playerinterfacegen.py
	if ! ./playerinterfacegen.py > player_interfaces.h; then rm player_interfaces.h; fi

interface_table.c: ./interfaces/* playerinterfacegen.py
	if ! ./playerinterfacegen.py --utils > interface_table.c; then rm interface_table.c; fi

