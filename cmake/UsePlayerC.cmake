CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)
INCLUDE (PlayerUtils)

INCLUDE (FindPkgConfig)
IF (NOT PKG_CONFIG_FOUND)
    #MESSAGE (FATAL_ERROR "Could not find pkg-config.")
    SET (PLAYERC_CFLAGS_STR "-I@CMAKE_INSTALL_PREFIX@/include/player@PLAYER_MAJOR_VERSION@.@PLAYER_MINOR_VERSION@")
    SET (PLAYERC_LDFLAGS_STR "-L@CMAKE_INSTALL_PREFIX@/Player/lib" "-lplayerc")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (PLAYERC playerc)
    IF (NOT PLAYERC_FOUND)
        MESSAGE (FATAL_ERROR "Could not find playerc with pkg-config.")
    ENDIF (NOT PLAYERC_FOUND)
ENDIF (NOT PKG_CONFIG_FOUND)
LIST_TO_STRING (PLAYERC_CFLAGS_STR "${PLAYERC_CFLAGS}")
LIST_TO_STRING (PLAYERC_LDFLAGS_STR "${PLAYERC_LDFLAGS}")


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

    IF (_includeDirs OR PLAYERC_INCLUDE_DIRS)
        INCLUDE_DIRECTORIES (${_includeDirs} ${PLAYERC_INCLUDE_DIRS})
    ENDIF (_includeDirs OR PLAYERC_INCLUDE_DIRS)
    IF (_libDirs OR PLAYERC_LIBRARY_DIRS)
        LINK_DIRECTORIES (${_libDirs} ${PLAYERC_LIBRARY_DIRS})
    ENDIF (_libDirs OR PLAYERC_LIBRARY_DIRS)

    ADD_EXECUTABLE (${_clientName} ${_srcs})
    SET_TARGET_PROPERTIES (${_clientName} PROPERTIES
        LINK_FLAGS ${PLAYERC_LDFLAGS_STR} ${_linkFlags}
        INSTALL_RPATH ${PLAYERC_LIBDIR}
        BUILD_WITH_INSTALL_RPATH TRUE)

    # Get the current cflags for each source file, and add the global ones
    # (this allows the user to specify individual cflags for each source file
    # without the global ones overriding them).
    IF (PLAYERC_CFLAGS_STR OR _cFLags)
        FOREACH (_file ${_srcs})
            GET_SOURCE_FILE_PROPERTY (_fileCFlags ${_file} COMPILE_FLAGS)
            IF (_fileCFlags STREQUAL NOTFOUND)
                SET (_newCFlags "${PLAYERC_CFLAGS_STR} ${_cFlags}")
            ELSE (_fileCFlags STREQUAL NOTFOUND)
                SET (_newCFlags "${_fileCFlags} ${PLAYERC_CFLAGS_STR} ${_cFlags}")
            ENDIF (_fileCFlags STREQUAL NOTFOUND)
            SET_SOURCE_FILES_PROPERTIES (${_file} PROPERTIES
                COMPILE_FLAGS ${_newCFlags})
        ENDFOREACH (_file)
    ENDIF (PLAYERC_CFLAGS_STR OR _cFLags)
ENDMACRO (PLAYER_ADD_PLAYERC_CLIENT)
