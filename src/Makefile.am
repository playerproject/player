AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = player

lib_LIBRARIES = libplayerqueue.a

libplayerqueue_a_SOURCES = playerqueue.cc

player_SOURCES = clientdata.cc pubsub_util.cc devicetable.cc device.cc \
              deviceregistry.cc clientmanager.cc wallclocktime.cc main.cc \
              playerqueue.cc configfile.cc

EXTRA_player_SOURCES = visiondevice.cc positiondevice.cc gripperdevice.cc \
                       laserbeacondevice.cc broadcastdevice.cc \
                       speechdevice.cc miscdevice.cc ptzdevice.cc \
                       audiodevice.cc bpsdevice.cc laserdevice.cc \
                       sonardevice.cc p2osdevice.cc robot_params.cc \
                       packet.cc sip.cc stagetime.cc stagedevice.cc \
                       rwi_bumperdevice.cc rwidevice.cc rwi_laserdevice.cc \
		       rwi_positiondevice.cc rwi_powerdevice.cc \
		       rwi_sonardevice.cc

player_DEPENDENCIES   = @LASER_OBJ@ \
                        @VISION_OBJ@ \
                        @PTZ_OBJ@ \
                        @AUDIO_OBJ@ \
                        @LASERBEACON_OBJ@ \
                        @BROADCAST_OBJ@ \
                        @SPEECH_OBJ@ \
                        @BPS_OBJ@ \
                        @P2OS_OBJ@ \
                        @RWI_OBJ@ \
                        @STAGE_OBJ@

player_LDADD   = -lpthread -lm -ldl @LASER_OBJ@ @VISION_OBJ@ @PTZ_OBJ@ @AUDIO_OBJ@ @LASERBEACON_OBJ@ @BROADCAST_OBJ@ @SPEECH_OBJ@ @BPS_OBJ@ @P2OS_OBJ@ @RWI_OBJ@ @STAGE_OBJ@

include_HEADERS = $(top_builddir)/include/defaults.h \
                  $(top_builddir)/include/playercommon.h \
                  $(top_builddir)/include/player.h \
                  $(top_builddir)/include/messages.h \
                  $(top_builddir)/include/playerqueue.h \
                  $(top_builddir)/include/stage.h

