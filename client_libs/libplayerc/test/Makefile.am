if WITH_TESTS
BUILDTEST = test
else
BUILDTEST =
endif

AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/server
LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm
EXTRA_DIST = test.world test.cfg cave.pnm

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
               test_truth.c \
               test_wifi.c \
							 test_localize.c

clean-local:
	rm -f test
