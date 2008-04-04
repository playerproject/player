# Global options that control the compile and are configurable by the user

SET (DEBUG_LEVEL NONE CACHE STRING "Level of debug code to be compiled: none, low, medium or high")

IF (WITH_GTK)
    OPTION (INCLUDE_RTKGUI "Include the RTK GUI in the server." OFF)
ENDIF (WITH_GTK)

OPTION (BUILD_SHARED_LIBS "Build the Player libraries as shared libraries." ON)