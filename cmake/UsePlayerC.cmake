CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)

INCLUDE (UsePkgConfig)
PKGCONFIG (playerc PLAYERC_INC_DIR PLAYERC_LINK_DIR PLAYERC_LINK_FLAGS PLAYERC_CFLAGS)
IF (NOT PLAYERC_CFLAGS)
    MESSAGE (FATAL_ERROR "playerc pkg-config not found")
ENDIF (NOT PLAYERC_CFLAGS)

# Get the lib dir from pkg-config to use as the rpath
FIND_PROGRAM (PKGCONFIG_EXECUTABLE NAMES pkg-config PATHS /usr/local/bin)
EXECUTE_PROCESS (COMMAND ${PKGCONFIG_EXECUTABLE} --variable=libdir playerc
    OUTPUT_VARIABLE PLAYERC_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)


###############################################################################
# Macro to build a simple client. Pass all source files as extra arguments.
# _clientName: The name of the executable to create
# _includeDirs, _libDirs, _linkFlags, _cFlags: extra include directories, lib
# directories, global link flags and global compile flags necessary to compile
# the client.
MACRO (PLAYER_ADD_PLAYERC_CLIENT _clientName _includeDirs _libDirs _linkFlags _cFLags)
    IF (NOT ${ARGC} GREATER 5)
        MESSAGE (FATAL_ERROR "No sources specified to PLAYER_ADD_PLAYERC_CLIENT. Did you remember to include blank strings for unused arguments?")
    ENDIF (NOT ${ARGC} GREATER 5)

    IF (_includeDirs OR PLAYERC_INC_DIR)
        INCLUDE_DIRECTORIES (${_includeDirs} ${PLAYERC_INC_DIR})
    ENDIF (_includeDirs OR PLAYERC_INC_DIR)
    IF (_libDirs OR PLAYERC_LINK_DIR)
        LINK_DIRECTORIES (${_libDirs} ${PLAYERC_LINK_DIR})
    ENDIF (_libDirs OR PLAYERC_LINK_DIR)

    ADD_EXECUTABLE (${_clientName} ${ARGN})
    SET_TARGET_PROPERTIES (${_clientName} PROPERTIES
        LINK_FLAGS ${PLAYERC_LINK_FLAGS} ${_linkFlags}
        INSTALL_RPATH ${PLAYERC_LIBDIR}
        BUILD_WITH_INSTALL_RPATH TRUE)

    # Get the current cflags for each source file, and add the global ones
    # (this allows the user to specify individual cflags for each source file
    # without the global ones overriding them).
    IF (PLAYERC_CFLAGS OR _cFLags)
        FOREACH (_file ${ARGN})
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
