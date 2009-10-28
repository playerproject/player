CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)
IF (COMMAND CMAKE_POLICY)
    IF (POLICY CMP0003)
        CMAKE_POLICY (SET CMP0003 NEW)
    ENDIF (POLICY CMP0003)
    IF (POLICY CMP0011)
        CMAKE_POLICY (SET CMP0011 NEW)
    ENDIF (POLICY CMP0011)
ENDIF (COMMAND CMAKE_POLICY)

INCLUDE (PlayerUtils)

INCLUDE (FindPkgConfig)
IF (NOT PKG_CONFIG_FOUND)
    SET (PLAYERCPP_CFLAGS "")
    SET (PLAYERCPP_INCLUDE_DIRS @PLAYERCC_EXTRA_INCLUDE_DIRS@)
    LIST (APPEND PLAYERCPP_INCLUDE_DIRS "@CMAKE_INSTALL_PREFIX@/include/player-@PLAYER_MAJOR_VERSION@.@PLAYER_MINOR_VERSION@")
    SET (PLAYERCPP_LINK_LIBS @PLAYERCC_EXTRA_LINK_LIBRARIES@)
    LIST (APPEND PLAYERCPP_LINK_LIBS "playerc++")
    SET (PLAYERCPP_LIBRARY_DIRS @PLAYERCC_EXTRA_LINK_DIRS@)
    LIST (APPEND PLAYERCPP_LIBRARY_DIRS "@CMAKE_INSTALL_PREFIX@/@PLAYER_LIBRARY_INSTALL_DIR@")
    SET (PLAYERCPP_LINK_FLAGS "")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (PLAYERCPP_PKG REQUIRED playerc++)
    IF (PLAYERCPP_PKG_CFLAGS_OTHER)
        LIST_TO_STRING (PLAYERCPP_CFLAGS "${PLAYERCPP_PKG_CFLAGS_OTHER}")
    ELSE (PLAYERCPP_PKG_CFLAGS_OTHER)
        SET (PLAYERCPP_CFLAGS "")
    ENDIF (PLAYERCPP_PKG_CFLAGS_OTHER)
    SET (PLAYERCPP_INCLUDE_DIRS ${PLAYERCPP_PKG_INCLUDE_DIRS})
    SET (PLAYERCPP_LINK_LIBS ${PLAYERCPP_PKG_LIBRARIES})
    SET (PLAYERCPP_LIBRARY_DIRS ${PLAYERCPP_PKG_LIBRARY_DIRS})
    IF (PLAYERCPP_PKG_LDFLAGS_OTHER)
        LIST_TO_STRING (PLAYERCPP_LINK_FLAGS ${PLAYERCPP_PKG_LDFLAGS_OTHER})
    ELSE (PLAYERCPP_PKG_LDFLAGS_OTHER)
        SET (PLAYERCPP_LINK_FLAGS "")
    ENDIF (PLAYERCPP_PKG_LDFLAGS_OTHER)
ENDIF (NOT PKG_CONFIG_FOUND)


###############################################################################
# Macro to build a simple client.
# _clientName: The name of the executable to create
# Pass source files, flags, etc. as extra args preceded by keywords as follows:
# SOURCES <source file list>
# INCLUDEDIRS <include directories list>
# LIBDIRS <library directories list>
# LINKFLAGS <link flags list>
# CFLAGS <compile flags list>
# See the examples directory (typically, ${prefix}/share/player/examples) for
# example CMakeLists.txt files.
MACRO (PLAYER_ADD_PLAYERCPP_CLIENT _clientName)
    PLAYER_PROCESS_ARGUMENTS (_srcs _includeDirs _libDirs _linkLibs _linkFlags _cFlags _junk ${ARGN})
    IF (_junk)
        MESSAGE (STATUS "WARNING: unkeyworded arguments found in PLAYER_ADD_PLAYERCPP_CLIENT: ${_junk}")
    ENDIF (_junk)
    LIST_TO_STRING (_cFlags "${_cFlags}")

    IF (_includeDirs OR PLAYERCPP_INCLUDE_DIRS)
        INCLUDE_DIRECTORIES (${_includeDirs} ${PLAYERCPP_INCLUDE_DIRS})
    ENDIF (_includeDirs OR PLAYERCPP_INCLUDE_DIRS)
    IF (_libDirs OR PLAYERCPP_LIBRARY_DIRS)
        LINK_DIRECTORIES (${_libDirs} ${PLAYERCPP_LIBRARY_DIRS})
    ENDIF (_libDirs OR PLAYERCPP_LIBRARY_DIRS)

    ADD_EXECUTABLE (${_clientName} ${_srcs})
    IF (PLAYERCPP_LIBRARY_DIRS)
        SET_TARGET_PROPERTIES (${_clientName} PROPERTIES
            INSTALL_RPATH ${PLAYERCPP_LIBRARY_DIRS}
            BUILD_WITH_INSTALL_RPATH TRUE)
    ENDIF (PLAYERCPP_LIBRARY_DIRS)
    IF (_linkFlags)
        LIST_TO_STRING (_linkFlags "${_linkFlags}")
        SET_TARGET_PROPERTIES (${_clientName} PROPERTIES LINK_FLAGS ${_linkFlags})
    ENDIF (_linkFlags)
    IF (PLAYERCPP_LINK_FLAGS)
        SET_TARGET_PROPERTIES (${_clientName} PROPERTIES LINK_FLAGS ${PLAYERCPP_LINK_FLAGS})
    ENDIF (PLAYERCPP_LINK_FLAGS)
    IF (_linkLibs)
        TARGET_LINK_LIBRARIES (${_clientName} ${_linkLibs})
    ENDIF (_linkLibs)
    IF (PLAYERCPP_LINK_LIBS)
        TARGET_LINK_LIBRARIES (${_clientName} ${PLAYERCPP_LINK_LIBS})
    ENDIF (PLAYERCPP_LINK_LIBS)

    # Get the current cflags for each source file, and add the global ones
    # (this allows the user to specify individual cflags for each source file
    # without the global ones overriding them).
    IF (PLAYERCPP_CFLAGS OR _cFlags)
        FOREACH (_file ${_srcs})
            GET_SOURCE_FILE_PROPERTY (_fileCFlags ${_file} COMPILE_FLAGS)
            IF (_fileCFlags STREQUAL NOTFOUND)
                SET (_newCFlags "${PLAYERCPP_CFLAGS} ${_cFlags}")
            ELSE (_fileCFlags STREQUAL NOTFOUND)
                SET (_newCFlags "${_fileCFlags} ${PLAYERCPP_CFLAGS} ${_cFlags}")
            ENDIF (_fileCFlags STREQUAL NOTFOUND)
            SET_SOURCE_FILES_PROPERTIES (${_file} PROPERTIES
                COMPILE_FLAGS ${_newCFlags})
        ENDFOREACH (_file)
    ENDIF (PLAYERCPP_CFLAGS OR _cFlags)
ENDMACRO (PLAYER_ADD_PLAYERCPP_CLIENT)
