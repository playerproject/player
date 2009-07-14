OPTION (BUILD_UTILS_PLAYERJOY "Build the playerjoy utility" ON)
IF (BUILD_UTILS_PLAYERJOY)
    IF (PLAYER_OS_WIN)
        MESSAGE (STATUS "playerjoy will not be built - termios and joystick are not supported under Windows")
    ELSE (PLAYER_OS_WIN)
        IF (BUILD_PLAYERCC)
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${PROJECT_BINARY_DIR}/client_libs)

            LINK_DIRECTORIES (${PLAYERCC_EXTRA_LINK_DIRS})
            PLAYER_ADD_EXECUTABLE (playerjoy playerjoy.cc)
            TARGET_LINK_LIBRARIES (playerjoy playerc++ playerc playerinterface
                                   playercommon ${PLAYERC_EXTRA_LINK_LIBRARIES}
                                   ${PLAYERCC_EXTRA_LINK_LIBRARIES} ${PTHREAD_LIB})
        ELSE (BUILD_PLAYERCC)
            MESSAGE (STATUS "playerjoy will not be built - playerc++ client library is disabled")
        ENDIF (BUILD_PLAYERCC)
    ENDIF (PLAYER_OS_WIN)
ENDIF (BUILD_UTILS_PLAYERJOY)
