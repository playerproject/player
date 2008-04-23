IF (WITH_GTK)
    SET (playervcrSrcs playervcr.c)

    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${GTK_INCLUDEDIR})
    PLAYER_ADD_EXECUTABLE (playervcr ${playervcrSrcs})
    TARGET_LINK_LIBRARIES (playervcr playerc playerxdr playererror playerutils ${PLAYERC_EXTRA_LINK_LIBRARIES})
    SET_SOURCE_FILES_PROPERTIES (${playervcrSrcs} PROPERTIES
        COMPILE_FLAGS "${GTK_CFLAGS}")
    SET_TARGET_PROPERTIES (playervcr PROPERTIES
        LINK_FLAGS "${GTK_LINKFLAGS}")
ELSE (WITH_GTK)
    MESSAGE (STATUS "playervcr will not be built - GTK not found")
ENDIF (WITH_GTK)