OPTION (BUILD_UTILS_PLAYERCAM "Build the playercam utility" ON)
IF (BUILD_UTILS_PLAYERCAM)
    IF (WITH_GTK AND WITH_GDKPIXBUF)
        SET (playercamSrcs playercam.c)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${GTK_PKG_INCLUDE_DIRS})
        LINK_DIRECTORIES (${GTK_PKG_LIBRARY_DIRS})
        PLAYER_ADD_EXECUTABLE (playercam ${playercamSrcs})
        TARGET_LINK_LIBRARIES (playercam playerc playerinterface playercommon
            ${PLAYERC_EXTRA_LINK_LIBRARIES} ${GTK_PKG_LIBRARIES})
        SET_SOURCE_FILES_PROPERTIES (${playercamSrcs} PROPERTIES
            COMPILE_FLAGS "${GTK_CFLAGS}")
        SET_TARGET_PROPERTIES (playercam PROPERTIES
            LINK_FLAGS "${GTK_LINKFLAGS}")
    ELSE (WITH_GTK AND WITH_GDKPIXBUF)
        IF (NOT WITH_GTK)
            MESSAGE (STATUS "playercam will not be built - GTK not found")
        ELSE (NOT WITH_GTK)
            MESSAGE (STATUS "playercam will not be built - GDK pixbuf not found")
        ENDIF (NOT WITH_GTK)
    ENDIF (WITH_GTK AND WITH_GDKPIXBUF)
ENDIF (BUILD_UTILS_PLAYERCAM)
