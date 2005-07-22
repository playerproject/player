AM_CPPFLAGS = -g -fPIC -Wall -D PLAYER_INSTALL_PREFIX="\"@PREFIX@\"" -I$(top_srcdir) 

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playercore.pc playererror.pc

EXTRA_DIST = playercore.pc.in playererror.pc.in

BUILT_SOURCES = playerconfig.h
CLEANFILES = playerconfig.h playercore.pc playererror.pc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LIBRARIES = libplayercore.a libplayererror.a

libplayercore_a_SOURCES = driver.cc driver.h \
                          device.cc device.h \
                          drivertable.h drivertable.cc \
                          devicetable.h devicetable.cc \
                          configfile.cc configfile.h \
                          playercommon.h player.h playerconfig.h.in \
                          message.h message.cc \
                          wallclocktime.cc wallclocktime.h playertime.h \
			  interface_util.cc interface_util.h \
			  plugins.cc plugins.h \
			  globals.cc globals.h

libplayererror_a_SOURCES = error.h error.c

coreincludedir = $(includedir)/player-@VERSION@/libplayercore
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
		      error.h
