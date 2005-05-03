SUBDIRS = . test bindings doc

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -I$(top_srcdir)/replace -I$(top_srcdir)/client_libs/libplayerpacket

# create the pkg-config entry	
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libplayerc.pc

lib_LIBRARIES = libplayerc.a

libplayerc_a_SOURCES = playerc.h \
                       mclient.c \
                       client.c \
                       device.c \
                       utils.c \
                       error.c \
                       error.h \
                       dev_blobfinder.c \
                       dev_bumper.c \
                       dev_camera.c \
                       dev_fiducial.c \
                       dev_gps.c \
                       dev_gripper.c \
                       dev_ir.c \
                       dev_joystick.c \
                       dev_laser.c \
                       dev_localize.c \
                       dev_log.c \
                       dev_map.c \
                       dev_motor.c \
                       dev_planner.c \
                       dev_position.c \
                       dev_position2d.c \
                       dev_position3d.c \
                       dev_power.c \
                       dev_ptz.c \
                       dev_sonar.c \
                       dev_truth.c \
                       dev_wifi.c \
		       dev_dio.c \
		       dev_simulation.c \
		       dev_gripper.c \
		       dev_speech.c

libplayerc_a_LIBADD = $(top_builddir)/replace/*.o

include_HEADERS = playerc.h
