if WITH_TESTS
BUILDTEST = test
else
BUILDTEST =
endif

AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../../c -I$(top_srcdir)/server
LDADD = -L$(top_builddir)/client_libs/c++ -lplayerclient

noinst_PROGRAMS = $(BUILDTEST)

test_SOURCES = test_bps.cc \
               test_broadcast.cc \
               test.cc \
               test_gps.cc \
               test_gripper.cc \
               test.h \
               test_laser.cc \
               test_lbd.cc \
               test_misc.cc \
               test_position.cc \
               test_ptz.cc \
               test_sonar.cc \
               test_speech.cc \
               test_truth.cc \
               test_vision.cc
clean-local:
	rm -f test
