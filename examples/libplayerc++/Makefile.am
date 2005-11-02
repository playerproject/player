PLAYERCC_CFLAGS = -I$(top_srcdir) \
                  -I$(top_srcdir)/client_libs/ \
                  $(PLAYERCC_EXTRA_CFLAGS)

PLAYERCC_LIB_DIRS=-L${top_builddir}/libplayerxdr \
                  -L${top_builddir}/client_libs/libplayerc \
                  -L${top_builddir}/client_libs/libplayerc++

PLAYERCC_DEPS = $(top_builddir)/client_libs/libplayerc/libplayerc.la \
                $(top_builddir)/libplayerxdr/libplayerxdr.la \
                $(top_builddir)/client_libs/libplayerc++/libplayerc++.la

AM_CPPFLAGS = $(PLAYERCC_CFLAGS) -Wall -g

bin_PROGRAMS =
if INCLUDE_PLAYERCC
  bin_PROGRAMS += example0 \
                  grip \
                  ptz \
                  laserobstacleavoid \
                  randomwalk \
                  sonarobstacleavoid
endif

if INCLUDE_PLAYERCC_BOOST
  bin_PROGRAMS += example1 example3 goto
endif

if INCLUDE_BOOST_SIGNALS
  bin_PROGRAMS += example2
endif

example0_SOURCES = example0.cc
example0_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
example0_DEPENDENCIES = $(PLAYERCC_DEPS)

example1_SOURCES = example1.cc
example1_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
example1_DEPENDENCIES = $(PLAYERCC_DEPS)

example2_SOURCES = example2.cc
example2_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
example2_DEPENDENCIES = $(PLAYERCC_DEPS)

example3_SOURCES = example3.cc
example3_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
example3_DEPENDENCIES = $(PLAYERCC_DEPS)

goto_SOURCES = goto.cc
goto_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
goto_DEPENDENCIES = $(PLAYERCC_DEPS)

grip_SOURCES = grip.cc
grip_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
grip_DEPENDENCIES = $(PLAYERCC_DEPS)

laserobstacleavoid_SOURCES = laserobstacleavoid.cc
laserobstacleavoid_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
laserobstacleavoid_DEPENDENCIES = $(PLAYERCC_DEPS)

ptz_SOURCES = ptz.cc
ptz_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
ptz_DEPENDENCIES = $(PLAYERCC_DEPS)

randomwalk_SOURCES = randomwalk.cc
randomwalk_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
randomwalk_DEPENDENCIES = $(PLAYERCC_DEPS)

sonarobstacleavoid_SOURCES = sonarobstacleavoid.cc
sonarobstacleavoid_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++
sonarobstacleavoid_DEPENDENCIES = $(PLAYERCC_DEPS)
