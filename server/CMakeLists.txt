# Include the driver utils
INCLUDE (${PLAYER_CMAKE_DIR}/internal/DriverUtils.cmake)

MESSAGE (STATUS)
MESSAGE (STATUS "----- Processing driver make files -----")
ADD_SUBDIRECTORY (drivers)
MESSAGE (STATUS "----- Done -----")
ADD_SUBDIRECTORY (libplayerdrivers)

# Write out a report on the drivers to be built or not to be built (that is the question)
WRITE_DRIVER_REPORT ()

# Build the server app
# Extra include dir for the server (why do we need this directoy?)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR}/drivers/shell)

IF (NOT PTHREAD_INCLUDE_DIR STREQUAL "")
    INCLUDE_DIRECTORIES (${PTHREAD_INCLUDE_DIR})
ENDIF (NOT PTHREAD_INCLUDE_DIR STREQUAL "")
IF (NOT PTHREAD_LIB_DIR STREQUAL "")
    LINK_DIRECTORIES (${PTHREAD_LIB_DIR})
ENDIF (NOT PTHREAD_LIB_DIR STREQUAL "")
IF (NOT HAVE_GETOPT)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
    SET (playerreplaceLib playerreplace)
ENDIF (NOT HAVE_GETOPT)
IF (PLAYER_DRIVERSLIB_LIBDIRS)
    FILTER_EMPTY (driverLinkDirs "${PLAYER_DRIVERSLIB_LIBDIRS}")
    FILTER_DUPLICATES (driverLinkDirs "${driverLinkDirs}")
    LINK_DIRECTORIES (${driverLinkDirs})
ENDIF (PLAYER_DRIVERSLIB_LIBDIRS)

# Source files
SET (playerSrcs server.cc)
# Server executable
ADD_EXECUTABLE (player ${playerSrcs})
TARGET_LINK_LIBRARIES (player playerdrivers playercore playercommon playertcp
                       playerudp playerinterface playerwkb ${playerreplaceLib}
                       ${PLAYERCORE_EXTRA_LINK_LIBRARIES})
IF (INCLUDE_RTKGUI)
    TARGET_LINK_LIBRARIES (player rtk)
ENDIF (INCLUDE_RTKGUI)
IF (HAVE_PLAYERSD)
    TARGET_LINK_LIBRARIES (player playersd)
ENDIF (HAVE_PLAYERSD)
IF (PLAYER_OS_SOLARIS)
    TARGET_LINK_LIBRARIES (player rt)
ENDIF (PLAYER_OS_SOLARIS)
STRING (REGEX REPLACE "^[ ]+" "" PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS}")
STRING (REGEX REPLACE "[ ]+$" "" PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS}")
IF (PLAYER_DRIVERSLIB_LINKFLAGS)
    TARGET_LINK_LIBRARIES (player ${PLAYER_DRIVERSLIB_LINKFLAGS})
ENDIF (PLAYER_DRIVERSLIB_LINKFLAGS)
INSTALL (TARGETS player DESTINATION bin/ COMPONENT applications)

# Clean up stuff from the drivers
PLAYERDRIVER_RESET_LISTS ()
