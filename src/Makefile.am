
INCLUDES = -I$(top_builddir)/include

bin_PROGRAMS = player

player_SOURCES = clientdata.cc pubsub_util.cc devicetable.cc device.cc \
              deviceregistry.cc clientmanager.cc wallclocktime.cc main.cc \
              playerqueue.cc configfile.cc

EXTRA_player_SOURCES = visiondevice.cc positiondevice.cc gripperdevice.cc \
                       laserbeacondevice.cc broadcastdevice.cc \
                       speechdevice.cc miscdevice.cc ptzdevice.cc \
                       audiodevice.cc bpsdevice.cc laserdevice.cc \
                       sonardevice.cc p2osdevice.cc robot_params.cc \
                       packet.cc sip.cc stagetime.cc stagedevice.cc

player_DEPENDENCIES   = @LASER_OBJ@ \
                        @VISION_OBJ@ \
                        @SONAR_OBJ@ \
                        @POSITION_OBJ@ \
                        @GRIPPER_OBJ@ \
                        @MISC_OBJ@ \
                        @PTZ_OBJ@ \
                        @AUDIO_OBJ@ \
                        @LASERBEACON_OBJ@ \
                        @BROADCAST_OBJ@ \
                        @SPEECH_OBJ@ \
                        @BPS_OBJ@ \
                        @P2OS_OBJ@ \
                        @STAGE_OBJ@

player_LDADD   = -lpthread -lm -ldl \
                 @LASER_OBJ@ \
                 @VISION_OBJ@ \
                 @SONAR_OBJ@ \
                 @POSITION_OBJ@ \
                 @GRIPPER_OBJ@ \
                 @MISC_OBJ@ \
                 @PTZ_OBJ@ \
                 @AUDIO_OBJ@ \
                 @LASERBEACON_OBJ@ \
                 @BROADCAST_OBJ@ \
                 @SPEECH_OBJ@ \
                 @BPS_OBJ@ \
                 @P2OS_OBJ@ \
                 @STAGE_OBJ@

