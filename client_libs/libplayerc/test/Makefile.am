if WITH_TESTS
BUILDTEST = test
else
BUILDTEST =
endif

AM_CPPFLAGS = -I$(srcdir)/.. -I$(top_srcdir)/server
LDADD = -L$(top_builddir)/client_libs/libplayerc -lplayerc -lm

noinst_PROGRAMS = $(BUILDTEST)

test_SOURCES = test.c \
							 test_broadcast.c \
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
