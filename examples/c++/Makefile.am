AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/client_libs/c -I$(top_srcdir)/client_libs/c++
LDADD = -L$(top_builddir)/client_libs/c++ -lplayerclient

noinst_PROGRAMS = rsn \
                  randomwalk \
	          visualservo \
                  sonarobstacleavoid \
	          goto \
	          ptz \
	          laserobstacleavoid \
                  example_from_manual \
	          say \
	          grip \
	          forage \
	          multilogger \
                  stress \
                  pherobot \
	          joystick
	          #multirandom multiphero

rsn_SOURCES = rsn.cc
randomwalk_SOURCES =  randomwalk.cc
visualservo_SOURCES = visualservo.cc
sonarobstacleavoid_SOURCES = sonarobstacleavoid.cc
goto_SOURCES = goto.cc
ptz_SOURCES = ptz.cc
laserobstacleavoid_SOURCES = laserobstacleavoid.cc
example_from_manual_SOURCES = example_from_manual.cc
say_SOURCES = say.cc
grip_SOURCES = grip.cc
forage_SOURCES = forage.cc
multilogger_SOURCES = multilogger.cc
stress_SOURCES = stress.cc
pherobot_SOURCES = pherobot.cc
joystick_SOURCES = joystick.cc

goto_LDADD = $(LDADD) -lpthread
joystick_LDADD = $(LDADD) -lpthread
