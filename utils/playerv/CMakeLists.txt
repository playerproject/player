OPTION (BUILD_UTILS_PLAYERV "Build the playerv utility" ON)
IF (BUILD_UTILS_PLAYERV)
    IF (WITH_GTK AND INCLUDE_RTK)
        SET (playervSrcs    playerv.c
                            playerv.h
                            registry.c
                            mainwnd.c
                            error.h
                            opt.c
                            opt.h
                            pv_dev_actarray.c
                            pv_dev_aio.c
                            pv_dev_blobfinder.c
                            pv_dev_camera.c
                            pv_dev_bumper.c
                            pv_dev_dio.c
                            pv_dev_fiducial.c
                            pv_dev_ir.c
                            pv_dev_laser.c
			    pv_dev_localize.c
                            pv_dev_map.c
                            pv_dev_position2d.c
                            pv_dev_power.c
                            pv_dev_ptz.c
                            pv_dev_sonar.c
                            pv_dev_wifi.c
                            pv_dev_gripper.c
                            pv_dev_ranger.c
                            pv_dev_vectormap.c)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${GTK_PKG_INCLUDE_DIRS})
        LINK_DIRECTORIES (${GTK_PKG_LIBRARY_DIRS})
        PLAYER_ADD_EXECUTABLE (playerv ${playervSrcs})
        TARGET_LINK_LIBRARIES (playerv playerc playerinterface playercommon
            rtk ${PLAYERC_EXTRA_LINK_LIBRARIES} ${GTK_PKG_LIBRARIES})
        IF (PLAYER_OS_SOLARIS)
            TARGET_LINK_LIBRARIES (playerv rt)
        ENDIF (PLAYER_OS_SOLARIS)
        SET_SOURCE_FILES_PROPERTIES (${playervSrcs} PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS}")
        SET_TARGET_PROPERTIES (playerv PROPERTIES
            LINK_FLAGS "${GTK_LINKFLAGS}")
    ELSE (WITH_GTK AND INCLUDE_RTK)
        MESSAGE (STATUS "playerv will not be built - GTK not found or RTK disabled.")
    ENDIF (WITH_GTK AND INCLUDE_RTK)
ENDIF (BUILD_UTILS_PLAYERV)
