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

AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/server
LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm

noinst_PROGRAMS = $(BUILDTEST)

test_SOURCES = test.c \
               test.h \
               test_blobfinder.c \
							 test_comms.c \
               test_fiducial.c \
               test_laser.c \
               test_gps.c \
               test_position.c \
               test_ptz.c \
               test_sonar.c \
               test_truth.c

clean-local:
	rm -f test
