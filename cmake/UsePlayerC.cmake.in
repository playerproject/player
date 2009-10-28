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
    SET (PLAYERC_CFLAGS "")
    SET (PLAYERC_INCLUDE_DIRS @PLAYERC_EXTRA_INCLUDE_DIRS@)
    LIST (APPEND PLAYERC_INCLUDE_DIRS "@CMAKE_INSTALL_PREFIX@/include/player-@PLAYER_MAJOR_VERSION@.@PLAYER_MINOR_VERSION@")
    SET (PLAYERC_LINK_LIBS @PLAYERC_EXTRA_LINK_LIBRARIES@)
    LIST (APPEND PLAYERC_LINK_LIBS "playerc")
    SET (PLAYERC_LIBRARY_DIRS @PLAYERC_EXTRA_LINK_DIRS@)
    LIST (APPEND PLAYERC_LIBRARY_DIRS "@CMAKE_INSTALL_PREFIX@/@PLAYER_LIBRARY_INSTALL_DIR@")
    SET (PLAYERC_LINK_FLAGS "")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (PLAYERC_PKG REQUIRED playerc)
    IF (PLAYERC_PKG_CFLAGS_OTHER)
        LIST_TO_STRING (PLAYERC_CFLAGS "${PLAYERC_PKG_CFLAGS_OTHER}")
    ELSE (PLAYERC_PKG_CFLAGS_OTHER)
        SET (PLAYERC_CFLAGS_OTHER "")
    ENDIF (PLAYERC_PKG_CFLAGS_OTHER)
    SET (PLAYERC_INCLUDE_DIRS ${PLAYERC_PKG_INCLUDE_DIRS})
    SET (PLAYERC_LINK_LIBS ${PLAYERC_PKG_LIBRARIES})
    SET (PLAYERC_LIBRARY_DIRS ${PLAYERC_PKG_LIBRARY_DIRS})
    IF (PLAYERC_PKG_LDFLAGS_OTHER)
        LIST_TO_STRING (PLAYERC_LINK_FLAGS ${PLAYERC_PKG_LDFLAGS_OTHER})
    ELSE (PLAYERC_PKG_LDFLAGS_OTHER)
        SET (PLAYERC_LINK_FLAGS "")
    ENDIF (PLAYERC_PKG_LDFLAGS_OTHER)
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
MACRO (PLAYER_ADD_PLAYERC_CLIENT _clientName)
    PLAYER_PROCESS_ARGUMENTS (_srcs _includeDirs _libDirs _linkLibs _linkFlags _cFlags _junk ${ARGN})
    IF (_junk)
        MESSAGE (STATUS "WARNING: unkeyworded arguments found in PLAYER_ADD_PLAYERC_CLIENT: ${_junk}")
    ENDIF (_junk)
    LIST_TO_STRING (_cFlags "${_cFlags}")

    IF (_includeDirs OR PLAYERC_INCLUDE_DIRS)
        INCLUDE_DIRECTORIES (${_includeDirs} ${PLAYERC_INCLUDE_DIRS})
    ENDIF (_includeDirs OR PLAYERC_INCLUDE_DIRS)
    IF (_libDirs OR PLAYERC_LIBRARY_DIRS)
        LINK_DIRECTORIES (${_libDirs} ${PLAYERC_LIBRARY_DIRS})
    ENDIF (_libDirs OR PLAYERC_LIBRARY_DIRS)

    ADD_EXECUTABLE (${_clientName} ${_srcs})
    IF (PLAYERC_LIBRARY_DIRS)
        SET_TARGET_PROPERTIES (${_clientName} PROPERTIES
            INSTALL_RPATH ${PLAYERC_LIBRARY_DIRS}
            BUILD_WITH_INSTALL_RPATH TRUE)
    ENDIF (PLAYERC_LIBRARY_DIRS)
    IF (PLAYERC_LINK_FLAGS)
        SET_TARGET_PROPERTIES (${_clientName} PROPERTIES LINK_FLAGS ${PLAYERC_LINK_FLAGS})
    ENDIF (PLAYERC_LINK_FLAGS)
    IF (_linkFlags)
        LIST_TO_STRING (_linkFlags "${_linkFlags}")
        SET_TARGET_PROPERTIES (${_clientName} PROPERTIES LINK_FLAGS ${_linkFlags})
    ENDIF (_linkFlags)
    IF (_linkLibs)
        TARGET_LINK_LIBRARIES (${_clientName} ${_linkLibs})
    ENDIF (_linkLibs)
    IF (PLAYERC_LINK_LIBS)
        TARGET_LINK_LIBRARIES (${_clientName} ${PLAYERC_LINK_LIBS})
    ENDIF (PLAYERC_LINK_LIBS)

    # Get the current cflags for each source file, and add the global ones
    # (this allows the user to specify individual cflags for each source file
    # without the global ones overriding them).
    IF (PLAYERC_CFLAGS OR _cFLags)
        FOREACH (_file ${_srcs})
            GET_SOURCE_FILE_PROPERTY (_fileCFlags ${_file} COMPILE_FLAGS)
            IF (_fileCFlags STREQUAL NOTFOUND)
                SET (_newCFlags "${PLAYERC_CFLAGS} ${_cFlags}")
            ELSE (_fileCFlags STREQUAL NOTFOUND)
                SET (_newCFlags "${_fileCFlags} ${PLAYERC_CFLAGS} ${_cFlags}")
            ENDIF (_fileCFlags STREQUAL NOTFOUND)
            SET_SOURCE_FILES_PROPERTIES (${_file} PROPERTIES
                COMPILE_FLAGS ${_newCFlags})
        ENDFOREACH (_file)
    ENDIF (PLAYERC_CFLAGS OR _cFLags)
ENDMACRO (PLAYER_ADD_PLAYERC_CLIENT)
