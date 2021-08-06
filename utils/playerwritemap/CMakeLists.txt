OPTION (BUILD_UTILS_PLAYERWRITEMAP "Build the playerwritemap utility" ON)
IF (BUILD_UTILS_PLAYERWRITEMAP)
    IF (WITH_GDKPIXBUF)
        SET (playerwritemapSrcs playerwritemap.c)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${GDKPIXBUF_PKG_INCLUDE_DIRS})
        LINK_DIRECTORIES (${GDKPIXBUF_PKG_LIBRARY_DIRS})
        PLAYER_ADD_EXECUTABLE (playerwritemap ${playerwritemapSrcs})
        TARGET_LINK_LIBRARIES (playerwritemap playerc playerinterface playercommon
            ${PLAYERC_EXTRA_LINK_LIBRARIES} ${GDKPIXBUF_PKG_LIBRARIES})
        SET_SOURCE_FILES_PROPERTIES (${playerwritemapSrcs} PROPERTIES
            COMPILE_FLAGS "${GDKPIXBUF_CFLAGS}")
        SET_TARGET_PROPERTIES (playerwritemap PROPERTIES
            LINK_FLAGS "${GDKPIXBUF_LINKFLAGS}")
    ELSE (WITH_GDKPIXBUF)
        MESSAGE (STATUS "playerwritemap will not be built - GDK pixbuf not found")
    ENDIF (WITH_GDKPIXBUF)
ENDIF (BUILD_UTILS_PLAYERWRITEMAP)
