AM_CPPFLAGS = -I$(top_srcdir) \
              -I$(top_srcdir)/client_libs \
              -Wall -g $(PLAYERCC_EXTRA_CFLAGS)

test_LDADD = $(PLAYERCC_LIB)

noinst_PROGRAMS =
if INCLUDE_PLAYERCC
if WITH_TESTS
  noinst_PROGRAMS += test
endif
endif

test_SOURCES = test.cc \
               test.h \
               test_client.cc \
               test_camera.cc

#               test_gps.cc \
               test_blobfinder.cc \
               test_gripper.cc \
               test_laser.cc \
               test_lbd.cc \
               test_motor.cc \
               test_position.cc \
               test_position2d.cc \
               test_position3d.cc \
               test_position_control.cc \
               test_ptz.cc \
               test_sonar.cc \
               test_speech.cc \
               test_truth.cc \
               test_vision.cc \
               test_power.cc \
               test_dio.cc \
               test_bumper.cc \
               test_wifi.cc \
               test_log.cc \
               test_mcom.cc \
               test_fiducialfinder.cc \
               test_lookup.cc \
               test_audiodsp.cc \
               test_localize.cc \
               test_audiomixer.cc \
               test_blinkenlight.cc

               #test_idar.cc \
               #test_idarturret.cc
               #test_misc.cc
               #test_bps.cc

