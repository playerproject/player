INCLUDE (${PLAYER_CMAKE_DIR}/internal/ProcessInterfaces.cmake)

# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

SET (interfaceDefs interfaces/001_player.def interfaces/002_power.def interfaces/003_gripper.def
                    interfaces/004_position2d.def interfaces/005_sonar.def interfaces/006_laser.def
                    interfaces/007_blobfinder.def interfaces/008_ptz.def interfaces/009_audio.def
                    interfaces/010_fiducial.def interfaces/012_speech.def interfaces/013_gps.def
                    interfaces/014_bumper.def interfaces/020_dio.def interfaces/021_aio.def
                    interfaces/022_ir.def interfaces/023_wifi.def interfaces/025_localize.def
                    interfaces/026_mcom.def interfaces/030_position3d.def
                    interfaces/031_simulation.def interfaces/033_blinkenlight.def
                    interfaces/040_camera.def interfaces/042_map.def interfaces/044_planner.def
                    interfaces/045_log.def interfaces/049_joystick.def
                    interfaces/050_speech_recognition.def interfaces/051_opaque.def
                    interfaces/052_position1d.def interfaces/053_actarray.def
                    interfaces/054_limb.def interfaces/055_graphics2d.def interfaces/056_rfid.def
                    interfaces/057_wsn.def interfaces/058_graphics3d.def interfaces/059_health.def
                    interfaces/060_imu.def interfaces/061_pointcloud3d.def interfaces/062_ranger.def
                    interfaces/063_vectormap.def interfaces/064_blackboard.def)
SET (player_interfaces_h "${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h")
SET (interface_table_h "${CMAKE_CURRENT_BINARY_DIR}/interface_table.h")
PROCESS_INTERFACES ("" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${player_interfaces_h} ${interfaceDefs})
ADD_CUSTOM_TARGET (player_interfaces ALL
    DEPENDS ${player_interfaces_h}
    COMMENT "Generating player_interfaces.h")
PROCESS_INTERFACES ("--utils" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${interface_table_h} ${player_interfaces_h})
ADD_CUSTOM_TARGET (interface_table ALL
    DEPENDS ${interface_table_h}
    COMMENT "Generating interface_table.h")

# For now, we're generating our own copy of playerxdr.h (and .c as a side-effect) here.
# This can be fixed when the cross-directory dependency with libplayerxdr is removed.
SET (playerxdr_h "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.h")
SET (playerxdr_c "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.c")
ADD_CUSTOM_COMMAND (OUTPUT ${playerxdr_h} ${playerxdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerxdr/playerxdrgen.py -distro ${CMAKE_CURRENT_SOURCE_DIR}/player.h ${playerxdr_c} ${playerxdr_h} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h)

# Include from the binary dir to get player_interfaces.h and interface_table.h
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})

SET (playercoreSrcs driver.cc
                    device.cc
                    drivertable.cc
                    devicetable.cc
                    configfile.cc
                    filewatcher.cc
                    message.cc
                    wallclocktime.cc
                    plugins.cc
                    globals.cc
                    property.cpp)

# TODO: playerxdr should NOT be linked here; it's a bogus dependency coming
# from the fact that message cloning functions are auto-generated into
# playerxdr and used here.  Those functions should go into a separate
# library.
SET (coreLibs playerutils playererror playerxdr pthread)
IF (PLAYER_OS_QNX)
    SET (corePCLibs)
ELSE (PLAYER_OS_QNX)
    SET (corePCLibs "-lpthread")
    PLAYER_ADD_LINK_LIB (pthread)
ENDIF (PLAYER_OS_QNX)

IF (HAVE_CLOCK_GETTIME)
    SET (coreLibs ${coreLibs} rt)
    SET (corePCLibs "${corePCLibs} -lrt")
    PLAYER_ADD_LINK_LIB (rt)
ELSE (HAVE_CLOCK_GETTIME)
    SET (playercoreSrcs ${playercoreSrcs} ${PROJECT_SOURCE_DIR}/replace/clock_gettime.c)
ENDIF (HAVE_CLOCK_GETTIME)
IF (HAVE_LIBLTDL)
    IF (PLAYER_OS_QNX)
        SET (coreLibs ${coreLibs} ltdl)
        SET (corePCLibs "${corePCLibs} -lltdl")
        PLAYER_ADD_LINK_LIB (ltdl)
    ELSE (PLAYER_OS_QNX)
        SET (coreLibs ${coreLibs} ltdl dl)
        SET (corePCLibs "${corePCLibs} -lltdl -ldl")
        PLAYER_ADD_LINK_LIB (ltdl dl)
    ENDIF (PLAYER_OS_QNX)
ENDIF (HAVE_LIBLTDL)
IF (NOT HAVE_DIRNAME)
  SET (playercoreSrcs ${playercoreSrcs} ${PROJECT_SOURCE_DIR}/replace/dirname.c)
ENDIF (NOT HAVE_DIRNAME)

PLAYER_ADD_LIBRARY (playercore ${playercoreSrcs} ${player_interfaces_h} ${playerxdr_h})
TARGET_LINK_LIBRARIES (playercore ${coreLibs})
PLAYER_MAKE_PKGCONFIG ("playercore" "Player core library - part of the Player Project"
                       "playererror" "" "" "${corePCLibs}")


SET (playererrorSrcs error.c)
PLAYER_ADD_LIBRARY (playererror ${playererrorSrcs})
PLAYER_MAKE_PKGCONFIG ("playererror" "Player error reporting library - part of the Player Project"
                       "" "" "" "")


SET (playerutilsSrcs interface_util.c
                      addr_util.c)
PLAYER_ADD_LIBRARY (playerutils ${playerutilsSrcs} ${interface_table_h})
TARGET_LINK_LIBRARIES (playerutils playererror)
PLAYER_MAKE_PKGCONFIG ("playerutils" "Player utilities library - part of the Player Project"
                       "" "" "" "")

PLAYER_INSTALL_HEADERS (playercore addr_util.h
                                   configfile.h
                                   device.h
                                   devicetable.h
                                   driver.h
                                   drivertable.h
                                   error.h
                                   filewatcher.h
                                   globals.h
                                   interface_util.h
                                   message.h
                                   playercommon.h
                                   ${playerconfig_h}
                                   playercore.h
                                   player.h
                                   ${player_interfaces_h}
                                   playertime.h
                                   plugins.h
                                   property.h
                                   wallclocktime.h)
