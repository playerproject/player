# Look for XMMS
FIND_PROGRAM (XMMS NAMES xmms-config)
IF (XMMS)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)
    EXECUTE_PROCESS (COMMAND xmms-config --cflags
        OUTPUT_VARIABLE xmmsCFlags)
    EXECUTE_PROCESS (COMMAND xmms-config --libs
        OUTPUT_VARIABLE xmmsLibs)
    ADD_LIBRARY (playerxmms STATIC playerxmms.c)
    TARGET_LINK_LIBRARIES (playerxmms playerc playerxdr playererror ${PLAYERC_EXTRA_LINK_LIBRARIES})
    SET_SOURCE_FILES_PROPERTIES (${playerxmms} PROPERTIES
        COMPILE_FLAGS "${xmmsCFlags}")
    SET_TARGET_PROPERTIES (playerxmms PROPERTIES
        LINK_FLAGS "${xmmsLibs}")
ELSE (XMMS)
    MESSAGE (STATUS "XMMS plugin will not be built - XMMS not found")
ENDIF (XMMS)