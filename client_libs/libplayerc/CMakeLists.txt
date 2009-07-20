ADD_SUBDIRECTORY (bindings)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)
# Possibly use our own copy of XDR, and need gettimeofday replacement
IF (NOT HAVE_XDR OR NOT HAVE_GETTIMEOFDAY)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
ENDIF (NOT HAVE_XDR OR NOT HAVE_GETTIMEOFDAY)

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
                    dev_joystick.c
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
                    dev_stereo.c
                    dev_vectormap.c
                    dev_wifi.c
                    dev_wsn.c)
IF (NOT HAVE_GETTIMEOFDAY)
    SET (playerreplaceLib playerreplace)
ENDIF (NOT HAVE_GETTIMEOFDAY)
IF (NOT HAVE_POLL)
    SET (playerreplaceLib playerreplace)
ENDIF (NOT HAVE_POLL)
IF (NOT HAVE_NANOSLEEP)
    SET (playerreplaceLib playerreplace)
ENDIF (NOT HAVE_NANOSLEEP)

IF (HAVE_M)
    PLAYERC_ADD_LINK_LIB (m)
ENDIF (HAVE_M)
IF (HAVE_Z)
    PLAYERC_ADD_LINK_LIB (z)
ENDIF (HAVE_Z)
IF (HAVE_GEOS)
    PLAYERC_ADD_LINK_LIB (geos geos_c)
ENDIF (HAVE_GEOS)
IF (PLAYER_OS_QNX)
    PLAYERC_ADD_LINK_LIB (socket rpc)
ELSEIF (PLAYER_OS_SOLARIS)
    PLAYERC_ADD_LINK_LIB (socket nsl rt)
ENDIF (PLAYER_OS_QNX)
IF (PLAYER_OS_WIN)
    PLAYERC_ADD_LINK_LIB (Ws2_32)
ENDIF (PLAYER_OS_WIN)

LINK_DIRECTORIES (${PLAYERC_EXTRA_LINK_DIRS})
PLAYER_ADD_LIBRARY (playerc ${playercSrcs})
ADD_DEPENDENCIES (playerc player_interfaces)
TARGET_LINK_LIBRARIES (playerc playerinterface playercommon playerwkb
                       ${playerreplaceLib} ${PLAYERC_EXTRA_LINK_LIBRARIES})
IF (HAVE_JPEG)
    TARGET_LINK_LIBRARIES (playerc playerjpeg)
ENDIF (HAVE_JPEG)

PLAYER_INSTALL_HEADERS (playerc playerc.h)

SET (pkgconfigCFlags)
SET (pkgconfigLinkDirs)
SET (pkgconfigLinkLibs)
LIST_TO_STRING_WITH_PREFIX (pkgconfigCFlags "-I" ${PLAYERC_EXTRA_INCLUDE_DIRS})
LIST_TO_STRING_WITH_PREFIX (pkgconfigLinkDirs "-L" ${PLAYERC_EXTRA_LINK_DIRS})
LIST_TO_STRING_WITH_PREFIX (pkgconfigLinkLibs "-l" ${PLAYERC_EXTRA_LINK_LIBRARIES})
PLAYER_MAKE_PKGCONFIG ("playerc"
                       "Andrew Howard's Player C client library - part of the Player Project"
                       "playerinterface playercommon playerwkb ${playerreplaceLib}" ""
                       "${pkgconfigCFlags}" "${pkgconfigLinkDirs} ${pkgconfigLinkLibs}")
