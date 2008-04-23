# Useful macros for building the Player libraries
INCLUDE (${PLAYER_CMAKE_DIR}/internal/Utils.cmake)

###############################################################################
# PLAYER_ADD_LIBRARY (_name)
# Adds a library to be built and installed and sets some common properties on it.
MACRO (PLAYER_ADD_LIBRARY _name)
#     MESSAGE (STATUS "Building library ${name} with sources ${ARGN}")
    ADD_LIBRARY (${_name} ${ARGN})
    SET_TARGET_PROPERTIES (${_name} PROPERTIES
                            VERSION ${PLAYER_VERSION}
                            SOVERSION ${PLAYER_API_VERSION}
                            INSTALL_RPATH "${INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/lib"
                            BUILD_WITH_INSTALL_RPATH TRUE)
    INSTALL (TARGETS ${_name} DESTINATION lib/)
ENDMACRO (PLAYER_ADD_LIBRARY)


###############################################################################
# PLAYER_ADD_EXECUTABLE (_name)
# Adds an executable to be built and installed and sets some common properties on it.
MACRO (PLAYER_ADD_EXECUTABLE _name)
#     MESSAGE (STATUS "Building library ${name} with sources ${ARGN}")
    ADD_EXECUTABLE (${_name} ${ARGN})
    SET_TARGET_PROPERTIES (${_name} PROPERTIES
                            INSTALL_RPATH "${INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/lib"
                            BUILD_WITH_INSTALL_RPATH TRUE)
    INSTALL (TARGETS ${_name} RUNTIME DESTINATION bin/)
ENDMACRO (PLAYER_ADD_EXECUTABLE)


###############################################################################
# PLAYER_ADD_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for stuff that links to the core libraries.
MACRO (PLAYER_ADD_LINK_LIB)
    SET (tempList ${PLAYER_EXTRA_LINK_LIBRARIES})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYER_EXTRA_LINK_LIBRARIES ${tempList} CACHE INTERNAL "Libs to link to" FORCE)
ENDMACRO (PLAYER_ADD_LINK_LIB)


###############################################################################
# PLAYERC_ADD_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for stuff that links to the core libraries.
MACRO (PLAYERC_ADD_LINK_LIB)
    SET (tempList ${PLAYERC_EXTRA_LINK_LIBRARIES})
    LIST (APPEND tempList ${ARGN})
    SET (PLAYERC_EXTRA_LINK_LIBRARIES ${tempList} CACHE INTERNAL "Libs to link to for playerc" FORCE)
ENDMACRO (PLAYERC_ADD_LINK_LIB)


###############################################################################
# PLAYERCC_ADD_LINK_LIB (library1 [library2 ...])
# Add libraries to the link line for stuff that links to the core libraries.
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
    INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/${_name}.pc DESTINATION lib/pkgconfig/)
ENDMACRO (PLAYER_MAKE_PKGCONFIG)


###############################################################################
# PLAYER_INSTALL_HEADERS
# Installs header files
MACRO (PLAYER_INSTALL_HEADERS _subdir)
    INSTALL (FILES ${ARGN}
        DESTINATION ${PLAYER_INCLUDE_INSTALL_DIR}/lib${_subdir})
ENDMACRO (PLAYER_INSTALL_HEADERS)
