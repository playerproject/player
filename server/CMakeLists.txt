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
# Source files
SET (playerSrcs server.cc)
# Server executable
ADD_EXECUTABLE (player ${playerSrcs})
TARGET_LINK_LIBRARIES (player playerdrivers playercore playererror playerutils playertcp playerudp playerxdr replace ${PLAYER_EXTRA_LINK_LIBRARIES})
IF (INCLUDE_RTKGUI)
    TARGET_LINK_LIBRARIES (player rtk)
ENDIF (INCLUDE_RTKGUI)
IF (HAVE_PLAYERSD)
    TARGET_LINK_LIBRARIES (player playersd)
ENDIF (HAVE_PLAYERSD)
STRING (REGEX REPLACE "^[ ]+" "" PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS}")
STRING (REGEX REPLACE "[ ]+$" "" PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS}")
IF (PLAYER_DRIVERSLIB_LINKFLAGS)
    TARGET_LINK_LIBRARIES (player ${PLAYER_DRIVERSLIB_LINKFLAGS})
ENDIF (PLAYER_DRIVERSLIB_LINKFLAGS)
INSTALL (TARGETS player DESTINATION bin/)

# Clean up stuff from the drivers
PLAYERDRIVER_RESET_LISTS ()
