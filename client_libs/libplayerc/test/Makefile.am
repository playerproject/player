if WITH_TESTS
BUILDTEST = test
else
BUILDTEST =
endif

AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/include
LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm

noinst_PROGRAMS = $(BUILDTEST)

test_SOURCES = test_bps.c \
               test_broadcast.c \
               test.c \
               test_gps.c \
               test.h \
               test_laser.c \
               test_lbd.c \
               test_position.c \
               test_ptz.c \
               test_sonar.c \
               test_truth.c \
               test_vision.c

clean-local:
	rm -f test
