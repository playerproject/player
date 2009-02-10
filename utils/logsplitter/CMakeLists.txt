OPTION (BUILD_UTILS_LOGSPLITTER "Build the logsplitter utility" ON)
IF (BUILD_UTILS_LOGSPLITTER)
    PLAYER_ADD_EXECUTABLE (playerlogsplitter logsplitter.c)
    IF (HAVE_M)
        TARGET_LINK_LIBRARIES (playerlogsplitter m)
    ENDIF (HAVE_M)
    # Wow, that one was easy!
ENDIF (BUILD_UTILS_LOGSPLITTER)
