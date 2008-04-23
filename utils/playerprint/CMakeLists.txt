IF (BUILD_PLAYERCC)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

    ADD_DEFINITIONS (${PLAYERCC_DEFINITIONS})
    PLAYER_ADD_EXECUTABLE (playerprint playerprint.cc)
    TARGET_LINK_LIBRARIES (playerprint playerc++ playerc playerxdr playererror ${PLAYERC_EXTRA_LINK_LIBRARIES} ${PLAYERCC_EXTRA_LINK_LIBRARIES})
ELSE (BUILD_PLAYERCC)
    MESSAGE (STATUS "playerprint will not be built - playerc++ client library is disabled")
ENDIF (BUILD_PLAYERCC)