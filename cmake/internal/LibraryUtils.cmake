# Useful macros for building the Player libraries
INCLUDE (${PLAYER_CMAKE_DIR}/PlayerUtils.cmake)

IF (PLAYER_OS_OSX)
    SET (RPATH_VAL ${CMAKE_INSTALL_PREFIX}/lib)
ELSE (PLAYER_OS_OSX)
    SET (RPATH_VAL "@rpath")
ENDIF (PLAYER_OS_OSX)

###############################################################################
# PLAYER_ADD_LIBRARY (_name)
# Adds a library to be built and installed and sets some common properties on it.
MACRO (PLAYER_ADD_LIBRARY _name)
    ADD_LIBRARY (${_name} ${ARGN})
    SET_TARGET_PROPERTIES (${_name} PROPERTIES
                            VERSION ${PLAYER_VERSION}
                            SOVERSION ${PLAYER_API_VERSION}
                            INSTALL_NAME_DIR ${RPATH_VAL}
                            INSTALL_RPATH "${INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/${PLAYER_LIBRARY_INSTALL_DIR}"
                            BUILD_WITH_INSTALL_RPATH TRUE)
    INSTALL (TARGETS ${_name} DESTINATION ${PLAYER_LIBRARY_INSTALL_DIR}/ COMPONENT libraries)
ENDMACRO (PLAYER_ADD_LIBRARY)


###############################################################################
# PLAYER_ADD_EXECUTABLE (_name)
# Adds an executable to be built and installed and sets some common properties on it.
MACRO (PLAYER_ADD_EXECUTABLE _name)
    ADD_EXECUTABLE (${_name} ${ARGN})
    SET_TARGET_PROPERTIES (${_name} PROPERTIES
                            INSTALL_RPATH "${INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/${PLAYER_LIBRARY_INSTALL_DIR}"
                            BUILD_WITH_INSTALL_RPATH TRUE)
    INSTALL (TARGETS ${_name} RUNTIME DESTINATION bin/ COMPONENT applications)
ENDMACRO (PLAYER_ADD_EXECUTABLE)


###############################################################################
# PLAYER_ADD_INCLUDE_DIR (dir1 [dir2 ...])
# Add include directories for stuff that uses the core libraries.
MACRO (PLAYER_ADD_INCLUDE_DIR)
    SET (tempList ${PLAYER_EXTRA_INCLUDE_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYER_EXTRA_INCLUDE_DIRS ${tempList} CACHE INTERNAL "Extra include directories" FORCE)
ENDMACRO (PLAYER_ADD_INCLUDE_DIR)


###############################################################################
# PLAYER_ADD_LINK_DIR (dir1 [dir2 ...])
# Add link directories for stuff that links to the core libraries.
MACRO (PLAYER_ADD_LINK_DIR)
    SET (tempList ${PLAYER_EXTRA_LINK_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYER_EXTRA_LINK_DIRS ${tempList} CACHE INTERNAL "Library directories to link in" FORCE)
ENDMACRO (PLAYER_ADD_LINK_DIR)


###############################################################################
# PLAYERCORE_ADD_INT_INCLUDE_DIR (dir1 [dir2 ...])
# Add include directories for the core libraries.
MACRO (PLAYERCORE_ADD_INT_INCLUDE_DIR)
    SET (tempList ${PLAYERCORE_INT_INCLUDE_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCORE_INT_INCLUDE_DIRS ${tempList} CACHE INTERNAL "Extra include directories for playercore" FORCE)
ENDMACRO (PLAYERCORE_ADD_INT_INCLUDE_DIR)


###############################################################################
# PLAYERCORE_ADD_INT_LINK_DIR (dir1 [dir2 ...])
# Add link directories for the core libraries.
MACRO (PLAYERCORE_ADD_INT_LINK_DIR)
    SET (tempList ${PLAYERCORE_INT_LINK_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCORE_INT_LINK_DIRS ${tempList} CACHE INTERNAL "Library directories to link playercore to" FORCE)
ENDMACRO (PLAYERCORE_ADD_INT_LINK_DIR)


###############################################################################
# PLAYERCORE_ADD_INT_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for the core libraries.
MACRO (PLAYERCORE_ADD_INT_LINK_LIB)
    SET (tempList ${PLAYERCORE_INT_LINK_LIBRARIES})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCORE_INT_LINK_LIBRARIES ${tempList} CACHE INTERNAL "Libs to link playercore to" FORCE)
ENDMACRO (PLAYERCORE_ADD_INT_LINK_LIB)


###############################################################################
# PLAYERCORE_ADD_INCLUDE_DIR (dir1 [dir2 ...])
# Add include directories for stuff that uses the core libraries.
MACRO (PLAYERCORE_ADD_INCLUDE_DIR)
    SET (tempList ${PLAYERCORE_EXTRA_INCLUDE_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCORE_EXTRA_INCLUDE_DIRS ${tempList} CACHE INTERNAL "Extra include directories with playercore" FORCE)
ENDMACRO (PLAYERCORE_ADD_INCLUDE_DIR)


###############################################################################
# PLAYERCORE_ADD_LINK_DIR (dir1 [dir2 ...])
# Add link directories for stuff that links to the core libraries.
MACRO (PLAYERCORE_ADD_LINK_DIR)
    SET (tempList ${PLAYERCORE_EXTRA_LINK_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCORE_EXTRA_LINK_DIRS ${tempList} CACHE INTERNAL "Library directories to link in with playercore" FORCE)
ENDMACRO (PLAYERCORE_ADD_LINK_DIR)


###############################################################################
# PLAYERCORE_ADD_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for stuff that links to the core libraries.
MACRO (PLAYERCORE_ADD_LINK_LIB)
    SET (tempList ${PLAYERCORE_EXTRA_LINK_LIBRARIES})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCORE_EXTRA_LINK_LIBRARIES ${tempList} CACHE INTERNAL "Libs to link to with playercore" FORCE)
ENDMACRO (PLAYERCORE_ADD_LINK_LIB)


###############################################################################
# PLAYERC_ADD_INCLUDE_DIR (dir1 [dir2 ...])
# Add include directories for stuff that uses the playerc libraries.
MACRO (PLAYERC_ADD_INCLUDE_DIR)
    SET (tempList ${PLAYERC_EXTRA_INCLUDE_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERC_EXTRA_INCLUDE_DIRS ${tempList} CACHE INTERNAL "Extra include directories when using playerc" FORCE)
ENDMACRO (PLAYERC_ADD_INCLUDE_DIR)


###############################################################################
# PLAYERC_ADD_LINK_DIR (dir1 [dir2 ...])
# Add link directories for stuff that links to the playerc libraries.
MACRO (PLAYERC_ADD_LINK_DIR)
    SET (tempList ${PLAYERC_EXTRA_LINK_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERC_EXTRA_LINK_DIRS ${tempList} CACHE INTERNAL "Library directories to link in when using playerc" FORCE)
ENDMACRO (PLAYERC_ADD_LINK_DIR)


###############################################################################
# PLAYERC_ADD_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for stuff that links to the playerc libraries.
MACRO (PLAYERC_ADD_LINK_LIB)
    SET (tempList ${PLAYERC_EXTRA_LINK_LIBRARIES})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERC_EXTRA_LINK_LIBRARIES ${tempList} CACHE INTERNAL "Libs to link to for using playerc" FORCE)
ENDMACRO (PLAYERC_ADD_LINK_LIB)


###############################################################################
# PLAYERCC_ADD_INCLUDE_DIR (dir1 [dir2 ...])
# Add include directories for stuff that uses the playerc++ libraries.
MACRO (PLAYERCC_ADD_INCLUDE_DIR)
    SET (tempList ${PLAYERCC_EXTRA_INCLUDE_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCC_EXTRA_INCLUDE_DIRS ${tempList} CACHE INTERNAL "Extra include directories when using playercc" FORCE)
ENDMACRO (PLAYERCC_ADD_INCLUDE_DIR)


###############################################################################
# PLAYERCC_ADD_LINK_DIR (dir1 [dir2 ...])
# Add link directories for stuff that links to the playerc++ libraries.
MACRO (PLAYERCC_ADD_LINK_DIR)
    SET (tempList ${PLAYERCC_EXTRA_LINK_DIRS})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCC_EXTRA_LINK_DIRS ${tempList} CACHE INTERNAL "Library directories to link in when using playercc" FORCE)
ENDMACRO (PLAYERCC_ADD_LINK_DIR)


###############################################################################
# PLAYERCC_ADD_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for stuff that links to the playerc++ libraries.
MACRO (PLAYERCC_ADD_LINK_LIB)
    SET (tempList ${PLAYERCC_EXTRA_LINK_LIBRARIES})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERCC_EXTRA_LINK_LIBRARIES ${tempList} CACHE INTERNAL "Libs to link to for playercc" FORCE)
ENDMACRO (PLAYERCC_ADD_LINK_LIB)


###############################################################################
# PLAYER_MAKE_PKGCONFIG
# Make a pkg-config file for a library
MACRO (PLAYER_MAKE_PKGCONFIG _name _desc _extDeps _intDeps _cFlags _libFlags)
    SET (PKG_NAME ${_name})
    SET (PKG_DESC ${_desc})
    SET (PKG_CFLAGS ${_cFlags})
    SET (PKG_LIBFLAGS ${_libFlags})
    SET (PKG_EXTERNAL_DEPS ${_extDeps})
    SET (PKG_INTERNAL_DEPS "")
    IF (${_intDeps})
        FOREACH (A ${_intDeps})
            SET (PKG_INTERNAL_DEPS "${PKG_INTERNAL_DEPS} -l${A}")
        ENDFOREACH (A ${_intDeps})
    ENDIF (${_intDeps})

    CONFIGURE_FILE (${PLAYER_CMAKE_DIR}/pkgconfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/${_name}.pc @ONLY)
    INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/${_name}.pc DESTINATION ${PLAYER_LIBRARY_INSTALL_DIR}/pkgconfig/ COMPONENT pkgconfig)
ENDMACRO (PLAYER_MAKE_PKGCONFIG)


###############################################################################
# PLAYER_INSTALL_HEADERS
# Installs header files
MACRO (PLAYER_INSTALL_HEADERS _subdir)
    INSTALL (FILES ${ARGN}
            DESTINATION ${PLAYER_INCLUDE_INSTALL_DIR}/lib${_subdir}
            COMPONENT headers)
ENDMACRO (PLAYER_INSTALL_HEADERS)


###############################################################################
# PLAYER_CLEAR_CACHED_LISTS ()
# Clears all the cached lists (prevents accumulation of the same value over and
# over).
MACRO (PLAYER_CLEAR_CACHED_LISTS)
    SET (PLAYER_EXTRA_INCLUDE_DIRS "" CACHE INTERNAL "Extra include directories" FORCE)
    SET (PLAYER_EXTRA_LINK_DIRS "" CACHE INTERNAL "Library directories to link in" FORCE)
    SET (PLAYERCORE_INT_INCLUDE_DIRS "" CACHE INTERNAL "Extra include directories for playercore" FORCE)
    SET (PLAYERCORE_INT_LINK_DIRS "" CACHE INTERNAL "Library directories to link playercore to" FORCE)
    SET (PLAYERCORE_INT_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link playercore to" FORCE)
    SET (PLAYERCORE_EXTRA_INCLUDE_DIRS "" CACHE INTERNAL "Extra include directories with playercore" FORCE)
    SET (PLAYERCORE_EXTRA_LINK_DIRS "" CACHE INTERNAL "Library directories to link in with playercore" FORCE)
    SET (PLAYERCORE_EXTRA_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link to with playercore" FORCE)
    SET (PLAYERC_EXTRA_INCLUDE_DIRS "" CACHE INTERNAL "Extra include directories when using playerc" FORCE)
    SET (PLAYERC_EXTRA_LINK_DIRS "" CACHE INTERNAL "Library directories to link in when using playerc" FORCE)
    SET (PLAYERC_EXTRA_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link to with playerc" FORCE)
    SET (PLAYERCC_EXTRA_INCLUDE_DIRS "" CACHE INTERNAL "Extra include directories when using playercc" FORCE)
    SET (PLAYERCC_EXTRA_LINK_DIRS "" CACHE INTERNAL "Library directories to link in when using playercc" FORCE)
    SET (PLAYERCC_EXTRA_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link to with playercc" FORCE)
ENDMACRO (PLAYER_CLEAR_CACHED_LISTS)

