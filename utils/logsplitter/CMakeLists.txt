OPTION (BUILD_UTILS_LOGSPLITTER "Build the logsplitter utility" ON)
IF (BUILD_UTILS_LOGSPLITTER)
    PLAYER_ADD_EXECUTABLE (playerlogsplitter logsplitter.c)
    # Wow, that one was easy!
ENDIF (BUILD_UTILS_LOGSPLITTER)
