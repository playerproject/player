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
