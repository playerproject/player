if WITH_TESTS
BUILDTEST = test
else
BUILDTEST =
endif

CC = @CC_CLIENTS@
CXX = @CXX_CLIENTS@
AR = @AR_CLIENTS@
RANLIB = @RANLIB_CLIENTS@
CXXFLAGS = @CXXFLAGS_CLIENTS@
CFLAGS = @CFLAGS_CLIENTS@

AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../../c -I$(top_srcdir)/server
LDADD = -L$(top_builddir)/client_libs/c++ -lplayerclient

noinst_PROGRAMS = $(BUILDTEST)

test_DEPENDENCIES = ../libplayerclient.a

test_SOURCES = test_broadcast.cc \
               test.cc \
               test_gps.cc \
               test_gripper.cc \
               test.h \
               test_laser.cc \
               test_lbd.cc \
               test_position.cc \
               test_ptz.cc \
               test_sonar.cc \
               test_speech.cc \
               test_truth.cc \
               test_vision.cc \
	       test_power.cc \
	       test_dio.cc \
               test_bumper.cc \
               test_wifi.cc 
               #test_idar.cc \
               #test_idarturret.cc
               #test_misc.cc
               #test_bps.cc

clean-local:
	rm -f test
