IF (BUILD_PLAYERCC)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${PROJECT_BINARY_DIR}/client_libs)

    PLAYER_ADD_EXECUTABLE (playerjoy playerjoy.cc)
    TARGET_LINK_LIBRARIES (playerjoy playerc++ playerc playerxdr playererror ${PLAYERC_EXTRA_LINK_LIBRARIES} ${PLAYERCC_EXTRA_LINK_LIBRARIES} pthread)
ELSE (BUILD_PLAYERCC)
    MESSAGE (STATUS "playerjoy will not be built - playerc++ client library is disabled")
ENDIF (BUILD_PLAYERCC)