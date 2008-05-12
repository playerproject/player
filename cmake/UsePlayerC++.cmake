CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)
INCLUDE (PlayerUtils)

INCLUDE (UsePkgConfig)
PKGCONFIG (playerc++ PLAYERCPP_INC_DIR PLAYERCPP_LINK_DIR PLAYERCPP_LINK_FLAGS PLAYERCPP_CFLAGS)
IF (NOT PLAYERCPP_CFLAGS)
    MESSAGE (FATAL_ERROR "playerc++ pkg-config not found")
ENDIF (NOT PLAYERCPP_CFLAGS)

# Get the lib dir from pkg-config to use as the rpath
FIND_PROGRAM (PKGCONFIG_EXECUTABLE NAMES pkg-config PATHS /usr/local/bin)
EXECUTE_PROCESS (COMMAND ${PKGCONFIG_EXECUTABLE} --variable=libdir playerc++
    OUTPUT_VARIABLE PLAYERCPP_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

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
    PLAYER_PROCESS_ARGUMENTS (_srcs _includeDirs _libDirs _linkFlags _cFlags _junk ${ARGN})
    IF (_junk)
        MESSAGE (STATUS "WARNING: unkeyworded arguments found in PLAYER_ADD_PLAYERCPP_CLIENT: ${_junk}")
    ENDIF (_junk)
    LIST_TO_STRING (_cFlags "${_cFlags}")

    IF (_includeDirs OR PLAYERCPP_INC_DIR)
        INCLUDE_DIRECTORIES (${_includeDirs} ${PLAYERC_INC_DIR})
    ENDIF (_includeDirs OR PLAYERCPP_INC_DIR)
    IF (_libDirs OR PLAYERCPP_LINK_DIR)
        LINK_DIRECTORIES (${_libDirs} ${PLAYERC_LINK_DIR})
    ENDIF (_libDirs OR PLAYERCPP_LINK_DIR)

    ADD_EXECUTABLE (${_clientName} ${_srcs})
    SET_TARGET_PROPERTIES (${_clientName} PROPERTIES
        LINK_FLAGS ${PLAYERCPP_LINK_FLAGS} ${_linkFlags}
        INSTALL_RPATH ${PLAYERCPP_LIBDIR}
        BUILD_WITH_INSTALL_RPATH TRUE)

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
            MESSAGE (STATUS "setting cflags to ${_newCFlags}")
            SET_SOURCE_FILES_PROPERTIES (${_file} PROPERTIES
                COMPILE_FLAGS ${_newCFlags})
        ENDFOREACH (_file)
    ENDIF (PLAYERCPP_CFLAGS OR _cFlags)
ENDMACRO (PLAYER_ADD_PLAYERCPP_CLIENT)
