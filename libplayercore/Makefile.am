AM_CPPFLAGS = -fPIC -Wall -D PLAYER_INSTALL_PREFIX="\"@PREFIX@\"" -I$(top_srcdir) -I$(top_srcdir)/server

# create the pkg-config entry for the server headers
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playercore.pc.in

BUILT_SOURCES = playerconfig.h playercore.pc
CLEANFILES = playerconfig.h playercore.pc

pkglibdir = $(libdir)/player-@VERSION@
pkglib_LIBRARIES = libplayercore.a

libplayercore_a_SOURCES = driver.cc driver.h \
                          device.cc device.h \
                          drivertable.h drivertable.cc \
                          devicetable.h devicetable.cc \
                          configfile.cc configfile.h \
                          playercommon.h player.h playerconfig.h.in \
                          error.h error.cc \
                          message.h message.cc \
                          wallclocktime.cc wallclocktime.h playertime.h \
			  interface_util.cc interface_util.h \
			  plugins.cc plugins.h \
			  globals.cc globals.h

pkgincludedir = $(includedir)/player-@VERSION@/libplayercore
pkginclude_HEADERS = configfile.h \
                     device.h \
                     devicetable.h \
                     driver.h \
                     drivertable.h \
                     error.h \
                     globals.h \
                     interface_util.h \
                     message.h \
                     player.h \
                     playercommon.h \
                     playerconfig.h \
                     playercore.h \
                     playertime.h \
                     plugins.h \
                     wallclocktime.h

