PLAYERCC_CFLAGS = -I$(top_srcdir) \
                  -I$(top_srcdir)/client_libs/ \
                  $(PLAYERCC_EXTRA_CFLAGS)

PLAYERCC_LIB_DIRS=-L${top_builddir}/libplayerxdr \
                  -L${top_builddir}/libplayercore \
                  -L${top_builddir}/client_libs/libplayerc \
                  -L${top_builddir}/client_libs/libplayerc++

PLAYERCC_DEPS = $(top_builddir)/client_libs/libplayerc/libplayerc.la \
                $(top_builddir)/libplayerxdr/libplayerxdr.la \
                $(top_builddir)/libplayercore/libplayererror.la \
                $(top_builddir)/client_libs/libplayerc++/libplayerc++.la

AM_CPPFLAGS = $(PLAYERCC_CFLAGS) -Wall -g

noinst_PROGRAMS =

if INCLUDE_PLAYERCC
  noinst_PROGRAMS += camera \
                     example0 \
                     grip \
                     ptz \
                     laserobstacleavoid \
                     randomwalk \
                     clientgraphics \
                     sonarobstacleavoid
endif

if INCLUDE_PLAYERCC_BOOST
  noinst_PROGRAMS += example1 example3 goto
endif

if INCLUDE_BOOST_SIGNALS
  noinst_PROGRAMS += example2
endif

camera_SOURCES = camera.cc args.h
camera_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
camera_DEPENDENCIES = $(PLAYERCC_DEPS)

example0_SOURCES = example0.cc
example0_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
example0_DEPENDENCIES = $(PLAYERCC_DEPS)

example1_SOURCES = example1.cc args.h
example1_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
example1_DEPENDENCIES = $(PLAYERCC_DEPS)

example2_SOURCES = example2.cc
example2_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
example2_DEPENDENCIES = $(PLAYERCC_DEPS)

example3_SOURCES = example3.cc args.h
example3_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
example3_DEPENDENCIES = $(PLAYERCC_DEPS)

goto_SOURCES = goto.cc
goto_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
goto_DEPENDENCIES = $(PLAYERCC_DEPS)

grip_SOURCES = grip.cc args.h
grip_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
grip_DEPENDENCIES = $(PLAYERCC_DEPS)

clientgraphics_SOURCES = clientgraphics.cc args.h
clientgraphics_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
clientgraphics_DEPENDENCIES = $(PLAYERCC_DEPS)

laserobstacleavoid_SOURCES = laserobstacleavoid.cc args.h
laserobstacleavoid_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
laserobstacleavoid_DEPENDENCIES = $(PLAYERCC_DEPS)

ptz_SOURCES = ptz.cc args.h
ptz_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
ptz_DEPENDENCIES = $(PLAYERCC_DEPS)

randomwalk_SOURCES = randomwalk.cc args.h
randomwalk_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
randomwalk_DEPENDENCIES = $(PLAYERCC_DEPS)

sonarobstacleavoid_SOURCES = sonarobstacleavoid.cc args.h
sonarobstacleavoid_LDADD = $(PLAYERCC_LIB_DIRS) $(PLAYERCC_LIB) -lplayerc++ -lplayerxdr -lplayererror
sonarobstacleavoid_DEPENDENCIES = $(PLAYERCC_DEPS)
