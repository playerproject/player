OPTION (BUILD_UTILS_PLAYERNAV "Build the playernav utility" ON)
IF (BUILD_UTILS_PLAYERNAV)
    IF (WITH_GNOMECANVAS)
        SET (playernavSrcs playernav.c gui.c player.c parse.c)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${GTK_INCLUDEDIR})
        PLAYER_ADD_EXECUTABLE (playernav ${playernavSrcs})
        TARGET_LINK_LIBRARIES (playernav playerc playerxdr playererror ${PLAYERC_EXTRA_LINK_LIBRARIES})
        SET_SOURCE_FILES_PROPERTIES (${playernavSrcs} PROPERTIES
            COMPILE_FLAGS "${GNOMECANVAS_CFLAGS}")
        SET_TARGET_PROPERTIES (playernav PROPERTIES
            LINK_FLAGS "${GNOMECANVAS_LINKFLAGS}")
    ELSE (WITH_GNOMECANVAS)
        MESSAGE (STATUS "playernav will not be built - Gnome Canvas not found")
    ENDIF (WITH_GNOMECANVAS)
ENDIF (BUILD_UTILS_PLAYERNAV)
