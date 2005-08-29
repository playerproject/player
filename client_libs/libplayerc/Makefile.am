SUBDIRS = . test 
#bindings doc

AM_CPPFLAGS = -Wall -I$(top_srcdir)

# TODO: figure out how to handle the jpeg wrappers
# -I$(top_srcdir)/client_libs/libplayerpacket

# create the pkg-config entry	
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = playerc.pc

EXTRA_DIST = playerc.pc.in
CLEANFILES = playerc.pc

lib_LTLIBRARIES = libplayerc.la

libplayerc_la_LDFLAGS = -version-info 2:0:0
libplayerc_la_SOURCES = playerc.h \
                        mclient.c \
                        client.c \
                        device.c \
                        utils.c \
                        error.c \
                        error.h \
                        dev_laser.c \
                        dev_log.c \
                        dev_position2d.c \
                        dev_position.c \
                        dev_simulation.c \
                        dev_sonar.c
#dev_blobfinder.c \
#dev_bumper.c \
#dev_camera.c \
#dev_fiducial.c \
#dev_gps.c \
#dev_gripper.c \
#dev_ir.c \
#dev_joystick.c \
#dev_localize.c \
#dev_map.c \
#dev_motor.c \
#dev_planner.c \
#dev_position3d.c \
#dev_power.c \
#dev_ptz.c \
#dev_truth.c \
#dev_wifi.c \
#dev_dio.c \
#dev_gripper.c \
#dev_speech.c

libplayerc_la_LIBADD = $(top_builddir)/replace/*.lo

playercincludedir = $(includedir)/libplayerc
playercinclude_HEADERS = playerc.h
