if WITH_TESTS
BUILDTEST = test
else
BUILDTEST =
endif

AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../../c -I$(top_srcdir)/server
test_LDADD = $(top_builddir)/client_libs/c++/libplayerclient.a

noinst_PROGRAMS = $(BUILDTEST)

test_SOURCES = test_blobfinder.cc \
               test.cc \
               test_gps.cc \
               test_gripper.cc \
               test.h \
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
               test_mcom.cc \
               test_fiducialfinder.cc \
               test_lookup.cc \
               test_audiodsp.cc \
               test_localize.cc \
               test_audiomixer.cc \
               test_blinkenlight.cc \
               test_camera.cc
               #test_idar.cc \
               #test_idarturret.cc
               #test_misc.cc
               #test_bps.cc

clean-local:
	rm -f test
