ADD_SUBDIRECTORY (bindings)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

SET (PLAYERC_EXTRA_LINK_LIBRARIES)
SET (playercSrcs    playerc.h
                    mclient.c
                    client.c
                    device.c
                    utils.c
                    error.c
                    error.h
                    dev_actarray.c
                    dev_aio.c
                    dev_audio.c
                    dev_blackboard.c
                    dev_blinkenlight.c
                    dev_bumper.c
                    dev_blobfinder.c
                    dev_camera.c
                    dev_dio.c
                    dev_fiducial.c
                    dev_gps.c
                    dev_graphics2d.c
                    dev_graphics3d.c
                    dev_gripper.c
                    dev_health.c
                    dev_imu.c
                    dev_ir.c
                    dev_laser.c
                    dev_limb.c
                    dev_localize.c
                    dev_log.c
                    dev_map.c
                    dev_opaque.c
                    dev_planner.c
                    dev_pointcloud3d.c
                    dev_position1d.c
                    dev_position2d.c
                    dev_position3d.c
                    dev_power.c
                    dev_ptz.c
                    dev_ranger.c
                    dev_rfid.c
                    dev_simulation.c
                    dev_sonar.c
                    dev_speech.c
                    dev_speech_recognition.c
                    dev_vectormap.c
                    dev_wifi.c
                    dev_wsn.c)
APPEND_TO_CACHED_LIST (PLAYERC_EXTRA_LINK_LIBRARIES "Libs to link to with playerc" z m)

PLAYER_ADD_LIBRARY (playerc ${playercSrcs})
ADD_DEPENDENCIES (playerc player_interfaces playerxdr_src)
TARGET_LINK_LIBRARIES (playerc playerxdr playerutils playererror ${PLAYERC_EXTRA_LINK_LIBRARIES})
IF (HAVE_LIBJPEG)
    TARGET_LINK_LIBRARIES (playerc playerjpeg)
    PLAYERC_ADD_LINK_LIB (jpeg)
ENDIF (HAVE_LIBJPEG)
IF (HAVE_GEOS)
    TARGET_LINK_LIBRARIES (playerc ${GEOS_LIBS})
ENDIF (HAVE_GEOS)

PLAYER_INSTALL_HEADERS (playerc playerc.h)

PLAYER_MAKE_PKGCONFIG ("playerc" "Andrew Howard's Player C client library - part of the Player Project" "playerxdr playererror" "" "" "-lm")