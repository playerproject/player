OPTION (BUILD_UTILS_PLAYERWRITEMAP "Build the playerwritemap utility" ON)
IF (BUILD_UTILS_PLAYERWRITEMAP)
    IF (WITH_GDKPIXBUF)
        SET (playerwritemapSrcs playerwritemap.c)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${GTK_INCLUDEDIR})
        PLAYER_ADD_EXECUTABLE (playerwritemap ${playerwritemapSrcs})
        TARGET_LINK_LIBRARIES (playerwritemap playerc playerxdr playererror playerutils ${PLAYERC_EXTRA_LINK_LIBRARIES})
        SET_SOURCE_FILES_PROPERTIES (${playerwritemapSrcs} PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS}")
        SET_TARGET_PROPERTIES (playerwritemap PROPERTIES
            LINK_FLAGS "${GTK_LINKFLAGS}")
    ELSE (WITH_GDKPIXBUF)
        MESSAGE (STATUS "playerwritemap will not be built - GDK pixbuf not found")
    ENDIF (WITH_GDKPIXBUF)
ENDIF (BUILD_UTILS_PLAYERWRITEMAP)
