AM_CPPFLAGS = -I$(top_srcdir)/server -I$(top_srcdir)/client_libs/c -I$(top_srcdir)/client_libs/c++
LDADD = $(top_builddir)/client_libs/c++/libplayerclient.a

noinst_PROGRAMS = randomwalk \
	          visualservo \
                  sonarobstacleavoid \
                  getcolor \
	          goto \
	          ptz \
	          laserobstacleavoid \
                  example_from_manual \
	          say \
	          grip \
                  stress \
                  pherobot \
		  audioread \
		  audiowrite \
                  waveformread \
                  audiodspread \
		  lookupport \
	          vfh foo

#multilogger
#forage

waveformread_SOURCES = waveformread.cc
audioread_SOURCES = audioread.cc
audiowrite_SOURCES = audiowrite.cc
randomwalk_SOURCES =  randomwalk.cc
visualservo_SOURCES = visualservo.cc
sonarobstacleavoid_SOURCES = sonarobstacleavoid.cc
getcolor_SOURCES = getcolor.cc
goto_SOURCES = goto.cc
ptz_SOURCES = ptz.cc
laserobstacleavoid_SOURCES = laserobstacleavoid.cc
example_from_manual_SOURCES = example_from_manual.cc
say_SOURCES = say.cc
grip_SOURCES = grip.cc
#forage_SOURCES = forage.cc
#multilogger_SOURCES = multilogger.cc
stress_SOURCES = stress.cc
pherobot_SOURCES = pherobot.cc
lookupport_SOURCES = lookupport.cc
audiodspread_SOURCES = audiodspread.cc
vfh_SOURCES = vfh.cc
foo_SOURCES = foo.cc
