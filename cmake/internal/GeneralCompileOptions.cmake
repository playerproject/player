# Global options that control the compile and are configurable by the user

SET (DEBUG_LEVEL NONE CACHE STRING "Level of debug code to be compiled: none, low, medium or high")

IF (WITH_GTK)
    OPTION (INCLUDE_RTK "Build the RTK GUI library." ON)
    IF (INCLUDE_RTK)
        OPTION (INCLUDE_RTKGUI "Include the RTK GUI in the server." OFF)
    ELSE (INCLUDE_RTK)
        SET (INCLUDE_RTKGUI OFF CACHE BOOL "Include the RTK GUI in the server." FORCE)
    ENDIF (INCLUDE_RTK)
ENDIF (WITH_GTK)

OPTION (BUILD_SHARED_LIBS "Build the Player libraries as shared libraries." ON)

IF (NOT PLAYER_OS_WIN)
    OPTION (LARGE_FILE_SUPPORT "Compile with support for large files (>2GB)." OFF)
    EXECUTE_PROCESS (COMMAND getconf LFS_CFLAGS OUTPUT_VARIABLE LFS_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF (LARGE_FILE_SUPPORT)
        MESSAGE (STATUS "Large file support is enabled, flags are ${LFS_FLAGS}")
        ADD_DEFINITIONS (${LFS_FLAGS})
    ELSE (LARGE_FILE_SUPPORT)
        MESSAGE (STATUS "Large file support is disabled.")
        REMOVE_DEFINITIONS (${LFS_FLAGS})
    ENDIF (LARGE_FILE_SUPPORT)
ENDIF (NOT PLAYER_OS_WIN)

