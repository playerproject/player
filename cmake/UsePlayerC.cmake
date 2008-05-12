CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)
INCLUDE (PlayerUtils)

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


##############################################################################
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
    PLAYER_PROCESS_ARGUMENTS (_srcs _includeDirs _libDirs _linkFlags _cFLags _junk ${ARGN})
    IF (_junk)
        MESSAGE (STATUS "WARNING: unkeyworded arguments found in PLAYER_ADD_PLAYERC_CLIENT: ${_junk}")
    ENDIF (_junk)
    LIST_TO_STRING (_cFlags "${_cFlags}")

    IF (_includeDirs OR PLAYERC_INC_DIR)
        INCLUDE_DIRECTORIES (${_includeDirs} ${PLAYERC_INC_DIR})
    ENDIF (_includeDirs OR PLAYERC_INC_DIR)
    IF (_libDirs OR PLAYERC_LINK_DIR)
        LINK_DIRECTORIES (${_libDirs} ${PLAYERC_LINK_DIR})
    ENDIF (_libDirs OR PLAYERC_LINK_DIR)

    ADD_EXECUTABLE (${_clientName} ${_srcs})
    SET_TARGET_PROPERTIES (${_clientName} PROPERTIES
        LINK_FLAGS ${PLAYERC_LINK_FLAGS} ${_linkFlags}
        INSTALL_RPATH ${PLAYERC_LIBDIR}
        BUILD_WITH_INSTALL_RPATH TRUE)

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
