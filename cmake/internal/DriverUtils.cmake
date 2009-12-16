# Macros for managing driver compiling

# Global maps are used to store the build info because we need the variables to be available
# in the parent. This has the unfortunate side-effect that it's also available everywhere
# due to being in the cache. The alternative is to make all driver CMakeLists.txt files
# INCLUDEd at the server/ level, but this would open up the worse possbility of
# cross-directory bugs between those files, which is risky when we're having driver authors
# maintain their own.

INCLUDE (${PLAYER_CMAKE_DIR}/PlayerUtils.cmake)

SET (PLAYER_BUILT_DRIVERS_DESC "List of drivers that will be built")
SET (PLAYER_BUILT_DRIVEREXTRAS_DESC "List of extra components for playerdrivers that will be built")
SET (PLAYER_DRIVERSLIB_SOURCES_MAP_DESC "Source files for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC "Include dirs for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_LIBDIRS_DESC "Lib dirs for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_LINKLIBS_DESC "Libraries for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_LINKFLAGS_DESC "Link flags for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_CFLAGS_DESC "Compile flags for libplayerdrivers")
SET (PLAYER_NOT_BUILT_DRIVERS_DESC "List of drivers that will not be built")
SET (PLAYER_NOT_BUILT_REASONS_DESC "Map of reasons drivers are not being built")
SET (PLAYERDRIVER_DEFINES_DESC "List of defines for driver_config.h")

###############################################################################
# PLAYERDRIVER_ADD_DRIVER (_name _cumulativeVar)
# Add a driver to the list of drivers to be built or not built.
# Only call this once you have determined the final value of cumulativeVar.
# Pass source files, flags, etc. as extra args preceded by keywords as follows:
# SOURCES <source file list>
# INCLUDEDIRS <include directories list>
# LIBDIRS <library directories list>
# LINKFLAGS <link flags list>
# CFLAGS <compile flags list>
MACRO (PLAYERDRIVER_ADD_DRIVER _name _cumulativeVar)
    IF (${_cumulativeVar})
        PLAYER_PROCESS_ARGUMENTS (_srcs _includeDirs _libDirs _linkLibs _linkFlags _cFlags _junk ${ARGN})
        IF (_junk)
            MESSAGE (STATUS "WARNING: Unkeyworded arguments found in PLAYERDRIVER_ADD_DRIVER: ${_junk}")
        ENDIF (_junk)
        LIST_TO_STRING (_cFlags "${_cFlags}")
        LIST_TO_STRING (_linkFlags "${_linkFlags}")
        IF (NOT _srcs)
            MESSAGE (STATUS "WARNING: No sources given for driver ${_name}")
        ENDIF (NOT _srcs)
        # Add this driver's list of sources to the list of sources for libplayerdrivers
        PLAYERDRIVER_ADD_TO_BUILT (${_name} "${_includeDirs}" "${_libDirs}" "${_linkLibs}" "${_linkFlags}" "${_cFlags}" ${_srcs})
    ENDIF (${_cumulativeVar})
ENDMACRO (PLAYERDRIVER_ADD_DRIVER)


###############################################################################
# PLAYERDRIVER_ADD_EXTRA (_name)
# Add some extra code to compile and link into playerdrivers.
# Pass source files, flags, etc. as extra args preceded by keywords as follows:
# SOURCES <source file list>
# INCLUDEDIRS <include directories list>
# LIBDIRS <library directories list>
# LINKFLAGS <link flags list>
# CFLAGS <compile flags list>
MACRO (PLAYERDRIVER_ADD_EXTRA _name)
    PLAYER_PROCESS_ARGUMENTS (_srcs _includeDirs _libDirs _linkLibs _linkFlags _cFlags _junk ${ARGN})
    IF (_junk)
        MESSAGE (STATUS "WARNING: Unkeyworded arguments found in PLAYERDRIVER_ADD_EXTRA: ${_junk}")
    ENDIF (_junk)
    LIST_TO_STRING (_cFlags "${_cFlags}")
    LIST_TO_STRING (_linkFlags "${_linkFlags}")
    # Add the list of sources to the list of sources for libplayerdrivers, and add various flags
    PLAYERDRIVER_ADD_EXTRA_TO_BUILT (${_name} "${_includeDir}" "${_libDirs}" "${_linkLibs}" "${_linkFlags}" "${_cFlags}" ${_srcs})
ENDMACRO (PLAYERDRIVER_ADD_EXTRA)


###############################################################################
# PLAYERDRIVER_OPTION (_name _cumulativeVar _defaultValue [reason])
# Add an option to enable/disable a driver.
# IMPORTANT: This macro initialises _cumulativeVar, so it must be called
# before any other PLAYERDRIVER macros.
#
# name:             Driver name.
# cumulativeVar:    The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# defaultValue:     Set to ON or OFF depending on if the driver should be built
#                   by default.
# reason:           Give a reason for disabling to the user. If empty, a
#                   standard reason will be given.
MACRO (PLAYERDRIVER_OPTION _name _cumulativeVar _defaultValue)
    # Default _cumulativeVar
    SET (${_cumulativeVar} TRUE)
    # Add an option for the driver
    PLAYERDRIVER_ADD_DRIVEROPTION (${_name} ${_defaultValue} 0)

    # Check if it's been disabled, if it has set _cumulativeVar and
    # add it to the list of reasons
    PLAYERDRIVER_MAKE_OPTION_NAME (optionName ${_name})
    IF (NOT ${optionName})
        SET (${_cumulativeVar} FALSE)
        # Check if the disabled reason has been customised
        IF (${ARGC} GREATER 3)
            SET (_reason ${ARGV3})
        ELSE (${ARGC} GREATER 3)
            # Check if there is already a reason for this driver to be disabled
            # (this prevents reconfigure from overwriting a previous reason).
            GET_FROM_GLOBAL_MAP (_prevReason PLAYER_NOT_BUILT_REASONS ${_name})
            IF ("${_prevReason}" STREQUAL "")
                SET (_reason "Disabled")
            ELSE ("${_prevReason}" STREQUAL "")
                SET (_reason "${_prevReason}")
            ENDIF ("${_prevReason}" STREQUAL "")
        ENDIF (${ARGC} GREATER 3)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "${_reason}")
    ENDIF (NOT ${optionName})
ENDMACRO (PLAYERDRIVER_OPTION)


###############################################################################
# PLAYERDRIVER_REQUIRE_OS (_name _cumulativeVar)
# Require a certain OS.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# variable args:    OS variables to check for (see FindOS.cmake in the
#                   cmake/internal/ dir).
MACRO (PLAYERDRIVER_REQUIRE_OS _name _cumulativeVar)
    IF (${_cumulativeVar})
        SET (isThisOS FALSE)
        FOREACH (osVar ${ARGN})
            IF (${osVar})
                SET (isThisOS TRUE)
            ENDIF (${osVar})
        ENDFOREACH (osVar ${ARGN})
        IF (NOT isThisOS)
            SET (${_cumulativeVar} FALSE)
            PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Cannot build on this operating system.")
            PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
        ENDIF (NOT isThisOS)
    ENDIF (${_cumulativeVar})
ENDMACRO (PLAYERDRIVER_REQUIRE_OS)


###############################################################################
# PLAYERDRIVER_REJECT_OS (_name _cumulativeVar)
# Prevent building on a certain OS.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# variable args:    OS variables to check for (see FindOS.cmake in the
#                   cmake/internal/ dir).
MACRO (PLAYERDRIVER_REJECT_OS _name _cumulativeVar)
    IF (${_cumulativeVar})
        FOREACH (osVar ${ARGN})
            IF (${osVar})
                SET (${_cumulativeVar} FALSE)
                PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Cannot build on this operating system.")
                PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
            ENDIF (${osVar})
        ENDFOREACH (osVar ${ARGN})
    ENDIF (${_cumulativeVar})
ENDMACRO (PLAYERDRIVER_REJECT_OS)


###############################################################################
# PLAYERDRIVER_REQUIRE_PKG (_name _cumulativeVar _package _includeDirs _libDirs _linkLibs _linkFlags _cFlags [_version])
# Check if a required package is available.
# If a minimum version is required, supply it as an optional argument with no spaces. For example,
# ">=0.9.6".
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _package:         Name (and possibly version) of the package to look for.
INCLUDE (FindPkgConfig)
MACRO (PLAYERDRIVER_REQUIRE_PKG _name _cumulativeVar _package _includeDirs _libDirs _linkLibs _linkFlags _cFlags)
    IF (NOT PKG_CONFIG_FOUND)
        MESSAGE (STATUS "Could not find pkg-config; cannot search for ${_package} for driver ${_name}.")
        IF (${_cumulativeVar})
            SET (${_cumulativeVar} FALSE)
            PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find pkg-config")
            PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
        ENDIF (${_cumulativeVar})
    ELSE (NOT PKG_CONFIG_FOUND)
        IF (${ARGC} GREATER 8)
            SET (_minVersion ${ARGV8})
        ENDIF (${ARGC} GREATER 8)

        # Look for the package using pkg-config
        SET (_pkgVar "${_package}_PKG")
        pkg_check_modules (${_pkgVar} ${_package}${_minVersion})

        # If not found, disable this driver
        # Dereference cumulativeVar only once because IF will dereference the variable name stored inside itself
        IF (${_cumulativeVar} AND ${_pkgVar}_FOUND)
            # Driver will be built and package found - add a #define
            STRING (TOUPPER ${_package} packageNameUpper)
            # Append to a list instead of setting an option so we can auto-generate driver_config.h
            APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_PKG_${_packageNameUpper}")
            # Set the values
            SET (${_includeDirs} ${${_pkgVar}_INCLUDE_DIRS})
            SET (${_libDirs} ${${_pkgVar}_LIBRARY_DIRS})
            SET (${_linkLibs} ${${_pkgVar}_LIBRARIES})
            LIST_TO_STRING (${_cFlags} "${${_pkgVar}_CFLAGS_OTHER}")
            LIST_TO_STRING (${_linkFlags} "${${_pkgVar}_LDFLAGS_OTHER}")
        ELSEIF (${_cumulativeVar})
            # Case where cumulativeVar is set but package wasn't found - don't build
            SET (${_cumulativeVar} FALSE)
            PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find package ${_package}")
            PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
        ENDIF (${_cumulativeVar} AND ${_pkgVar}_FOUND)
    ENDIF (NOT PKG_CONFIG_FOUND)
ENDMACRO (PLAYERDRIVER_REQUIRE_PKG)

###############################################################################
# PLAYERDRIVER_REQUIRE_HEADER (_name _cumulativeVar _header)
# Check if a required header file is available.
# If extra include directories are necessary for the check, set them using
# CMAKE_REQUIRED_INCLUDES. If extra include files must be included for the
# check to succeed, specify them as extra arguments.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _header:          Name of the header file to look for.
INCLUDE (CheckIncludeFiles)
MACRO (PLAYERDRIVER_REQUIRE_HEADER _name _cumulativeVar _header)
    IF (${PLAYER_EXTRA_INCLUDE_DIRS})
        LIST (APPEND CMAKE_REQUIRED_HEADERS ${PLAYER_EXTRA_INCLUDE_DIRS})
    ENDIF (${PLAYER_EXTRA_INCLUDE_DIRS})
    # Check for extra headers
    SET (headers)
    IF (${ARGC} GREATER 3)
        LIST (APPEND headers ${ARGN})
    ENDIF (${ARGC} GREATER 3)
    LIST (APPEND headers ${_header})
    STRING (TOUPPER ${_header} headerUpper)
    STRING (REGEX REPLACE "[./\\]" "_" headerUpper "${headerUpper}")
    SET (resultVar "HAVE_HDR_${headerUpper}")
    CHECK_INCLUDE_FILES ("${headers}" ${resultVar})
    # If not found, disable this driver
    # Dereference cumulativeVar only once because IF will dereference the variable name stored inside itself
    IF (${_cumulativeVar} AND ${resultVar})
        # Driver will be built and header found - add a #define
        # Append to a list instead of setting an option so we can auto-generate driver_config.h
        APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_HDR_${headerUpper}")
    ELSEIF (${_cumulativeVar})
        # Case where cumulativeVar is set but header wasn't found - don't build
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find header ${_header}")
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
    ENDIF (${_cumulativeVar} AND ${resultVar})
ENDMACRO (PLAYERDRIVER_REQUIRE_HEADER)


###############################################################################
# PLAYERDRIVER_REQUIRE_HEADER_CPP (_name _cumulativeVar _header)
# Check if a required C++ header file is available. (CMake will try to compile
#  a small program using c++ with an include<_header> line)
#
# See the CheckIncludeFileCXX.cmake module for extra variables that may modify
# how this macro runs.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _header:          Name of the header file to look for.
INCLUDE (CheckIncludeFileCXX)
MACRO (PLAYERDRIVER_REQUIRE_HEADER_CPP _name _cumulativeVar _header)
    IF (${PLAYER_EXTRA_INCLUDE_DIRS})
        LIST (APPEND CMAKE_REQUIRED_HEADERS ${PLAYER_EXTRA_INCLUDE_DIRS})
    ENDIF (${PLAYER_EXTRA_INCLUDE_DIRS})
    STRING (TOUPPER ${_header} headerUpper)
    STRING (REGEX REPLACE "[./\\]" "_" headerUpper "${headerUpper}")
    SET (resultVar "HAVE_HDR_${headerUpper}")
    CHECK_INCLUDE_FILE_CXX("${_header}" ${resultVar})
    # If not found, disable this driver
    # Dereference cumulativeVar only once because IF will dereference the variable name stored inside itself
    IF (${_cumulativeVar} AND ${resultVar})
        # Driver will be built and header found - add a #define
        # Append to a list instead of setting an option so we can auto-generate driver_config.h
        APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_HDR_${headerUpper}")
    ELSEIF (${_cumulativeVar})
        # Case where cumulativeVar is set but header wasn't found - don't build
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find header ${_header}")
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
    ENDIF (${_cumulativeVar} AND ${resultVar})
ENDMACRO (PLAYERDRIVER_REQUIRE_HEADER_CPP)


###############################################################################
# PLAYERDRIVER_REQUIRE_FUNCTION (_name _cumulativeVar _function)
# Check if a required function is available.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _function:        Name of the function to look for.
INCLUDE (CheckFunctionExists)
MACRO (PLAYERDRIVER_REQUIRE_FUNCTION _name _cumulativeVar _function)
    IF (${PLAYER_EXTRA_INCLUDE_DIRS})
        LIST (APPEND CMAKE_REQUIRED_HEADERS ${PLAYER_EXTRA_INCLUDE_DIRS})
    ENDIF (${PLAYER_EXTRA_INCLUDE_DIRS})

    SET (foundFunction)
    CHECK_FUNCTION_EXISTS (${_function} foundFunction)
    # If not found, disable this driver
    # Dereference cumulativeVar only once because IF will dereference the variable name stored inside itself
    IF (${_cumulativeVar} AND foundFunction)
        # Driver will be built and function found - add a #define
        STRING (TOUPPER ${_function} functionUpper)
        # Append to a list instead of setting an option so we can auto-generate driver_config.h
        APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_FUNC_${functionUpper}")
    ELSEIF (${_cumulativeVar})
        # Case where cumulativeVar is set but function wasn't found - don't build
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find function ${_function}")
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
    ENDIF (${_cumulativeVar} AND foundFunction)
ENDMACRO (PLAYERDRIVER_REQUIRE_FUNCTION)


###############################################################################
# PLAYERDRIVER_REQUIRE_LIB (_name _cumulativeVar _library _function _path)
# Check if a required library is available.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _library:         Name of the library to look for.
# _function:        Function to check the library with.
# _path:            Location where the library is expected to be.
INCLUDE (CheckLibraryExists)
MACRO (PLAYERDRIVER_REQUIRE_LIB _name _cumulativeVar _library _function _path)
    SET (foundLibrary)
    IF (PLAYER_EXTRA_LIB_DIRS AND NOT _path)
        SET (_path "${PLAYER_EXTRA_LIB_DIRS}")
    ELSEIF (NOT PLAYER_EXTRA_LIB_DIRS AND NOT _path)
        SET (_path ".")
    ENDIF (PLAYER_EXTRA_LIB_DIRS AND NOT _path)
    CHECK_LIBRARY_EXISTS ("${_library}" "${_function}" "${_path}" foundLibrary)
    # If not found, disable this driver
    # Dereference cumulativeVar only once because IF will dereference the variable name stored inside itself
    IF (${_cumulativeVar} AND foundLibrary)
        # Driver will be built and library found - add a #define
        STRING (TOUPPER ${_library} libraryUpper)
        # Append to a list instead of setting an option so we can auto-generate driver_config.h
        APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_LIB_${libraryUpper}")
    ELSEIF (${_cumulativeVar})
        # Case where cumulativeVar is set but library wasn't found - don't build
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find library ${_library}")
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
    ENDIF (${_cumulativeVar} AND foundLibrary)
ENDMACRO (PLAYERDRIVER_REQUIRE_LIB)


###############################################################################
# PLAYERDRIVER_REQUIRE_ENVVAR (_name _cumulativeVar _envvar _dest)
# Require an environment variable be set, and store its value in the variable
# stored in _dest.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _envvar:          The environment variable to check for.
# _dest:            The variable to store the value in. Set to NOTFOUND if the
#                   environment variable is not set.
MACRO (PLAYERDRIVER_REQUIRE_ENVVAR _name _cumulativeVar _envvar _dest)
    # Using ${_envvar} directly in $ENV{} results in a segfault under 2.6.4
    SET (_tempVarName ${_envvar})
    SET (${_dest} $ENV{${_tempVarName}})
    IF (NOT ${_dest})
        SET (${_dest} "NOTFOUND")
        IF (${_cumulativeVar})
            SET (${_cumulativeVar} FALSE)
            PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "${_envvar} was not set.")
            PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF 1)
        ENDIF (${_cumulativeVar})
    ENDIF (NOT ${_dest})
ENDMACRO (PLAYERDRIVER_REQUIRE_ENVVAR)


###############################################################################
# PLAYERDRIVER_ADD_DEFINEOPTION (_cumulativeVar _option _defaultValue _type _desc)
# Add an option that can be set to ON or OFF by the user and a #define set
# accordingly in driver_config.h. _name is the driver name. _type must be one
# of the CMake option types: BOOL, INT, STRING.
MACRO (PLAYERDRIVER_ADD_DEFINEOPTION _cumulativeVar _option _defaultValue _type _desc)
    # Check for duplicated options
    STRING_IN_LIST (_duplicate "${PLAYERDRIVER_DEFINES}" ${_option})
    IF (_duplicate)
        MESSAGE (SEND_ERROR "Duplicate driver define option: ${_option} (${_desc})")
    ELSE (_duplicate)
        IF (${_cumulativeVar})
            # Add an option for this value
            SET (${_option} ${_defaultValue} CACHE ${_type} ${_desc})
            # Record this option in the list so it can be added to driver_config.h later
            APPEND_TO_CACHED_LIST (PLAYERDRIVER_DEFINES ${PLAYERDRIVER_DEFINES_DESC} ${_option})
        ENDIF (${_cumulativeVar})
    ENDIF (_duplicate)
ENDMACRO (PLAYERDRIVER_ADD_DEFINEOPTION)


###############################################################################
# INTERNAL MACROS

###############################################################################
# PLAYERDRIVER_ADD_DRIVEROPTION (_name _value _force)
# Set the option for a driver to a value. If _force is true, the option
# will be forced to the value rather than the cache value being used. This
# is useful to update the ccmake GUI when an option is changed internally.
MACRO (PLAYERDRIVER_ADD_DRIVEROPTION _name _value _force)
    # Get the option name for this driver
    PLAYERDRIVER_MAKE_OPTION_NAME (optionName ${_name})
    IF (${_force})
        # Force the option value
        SET (${optionName} ${_value} CACHE BOOL "Build driver ${_name}" FORCE)
    ELSE (${_force})
        SET (${optionName} ${_value} CACHE BOOL "Build driver ${_name}")
    ENDIF (${_force})
ENDMACRO (PLAYERDRIVER_ADD_DRIVEROPTION)


###############################################################################
# Make an option name from a driver name.
MACRO (PLAYERDRIVER_MAKE_OPTION_NAME _optionName _driverName)
    STRING (TOUPPER ${_driverName} driverNameUpper)
    SET (${_optionName} "ENABLE_DRIVER_${driverNameUpper}")
ENDMACRO (PLAYERDRIVER_MAKE_OPTION_NAME)


###############################################################################
# Add a driver to the list of drivers to be built.
MACRO (PLAYERDRIVER_ADD_TO_BUILT _name _includeDir _libDirs _linkLibs _linkFlags _cFlags)
    # Source files go into a map so we can set the cflags on them later on
    SET (tempList)
    FOREACH (sourceFile ${ARGN})
        # If the path begins with a /, it's absolute so add it raw. Otherwise it's relative so
        # we need to add a full path to the front to make it absolute.
        IF (sourceFile MATCHES "^/.*")
            LIST (APPEND tempList ${sourceFile})
        ELSE (sourceFile MATCHES "^/.*")
            LIST (APPEND tempList "${CMAKE_CURRENT_SOURCE_DIR}/${sourceFile}")
        ENDIF (sourceFile MATCHES "^/.*")
    ENDFOREACH (sourceFile ${ARGN})
    INSERT_INTO_GLOBAL_MAP (PLAYER_DRIVERSLIB_SOURCES_MAP ${_name} "${tempList}")

    # Append the various build options
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_INCLUDEDIRS ${PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC} ${_includeDir})
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LIBDIRS ${PLAYER_DRIVERSLIB_LIBDIRS_DESC} ${_libDirs})
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LINKLIBS ${PLAYER_DRIVERSLIB_LINKLIBS_DESC} ${_linkLibs})
#     APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LINKFLAGS ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} ${_linkFlags})
    SET (PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS} ${_linkFlags}" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} FORCE)
    # C flags go into a map indexed by name because they're driver-specific
    INSERT_INTO_GLOBAL_MAP (PLAYER_DRIVERSLIB_CFLAGS ${_name} "${_cFlags}")

    # Set this driver to be built
    APPEND_TO_CACHED_LIST (PLAYER_BUILT_DRIVERS ${PLAYER_BUILT_DRIVERS_DESC} ${_name})
ENDMACRO (PLAYERDRIVER_ADD_TO_BUILT)


###############################################################################
# Add some extra code to playerdrivers.
MACRO (PLAYERDRIVER_ADD_EXTRA_TO_BUILT _name _includeDir _libDirs _linkLibs _linkFlags _cFlags)
    # Source files go into a map so we can set the cflags on them later on
    SET (tempList)
    FOREACH (sourceFile ${ARGN})
        LIST (APPEND tempList "${CMAKE_CURRENT_SOURCE_DIR}/${sourceFile}")
    ENDFOREACH (sourceFile ${ARGN})
    INSERT_INTO_GLOBAL_MAP (PLAYER_DRIVERSLIB_SOURCES_MAP ${_name} "${tempList}")

    # Append the various build options
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_INCLUDEDIRS ${PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC} ${_includeDir})
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LIBDIRS ${PLAYER_DRIVERSLIB_LIBDIRS_DESC} ${_libDirs})
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LINKLIBS ${PLAYER_DRIVERSLIB_LINKLIBS_DESC} ${_linkLibs})
    SET (PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS} ${_linkFlags}" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} FORCE)
    # C flags go into a map indexed by name because they're driver-specific
    INSERT_INTO_GLOBAL_MAP (PLAYER_DRIVERSLIB_CFLAGS ${_name} "${_cFlags}")

    # Set this driver to be built
    APPEND_TO_CACHED_LIST (PLAYER_BUILT_DRIVEREXTRAS ${PLAYER_BUILT_DRIVEREXTRAS_DESC} ${_name})
ENDMACRO (PLAYERDRIVER_ADD_EXTRA_TO_BUILT)


###############################################################################
# Add a driver to the list of drivers not to be built.
MACRO (PLAYERDRIVER_ADD_TO_NOT_BUILT _name _reason)
    APPEND_TO_CACHED_LIST (PLAYER_NOT_BUILT_DRIVERS ${PLAYER_NOT_BUILT_DRIVERS_DESC} ${_name})
    INSERT_INTO_GLOBAL_MAP (PLAYER_NOT_BUILT_REASONS ${_name} ${_reason})
ENDMACRO (PLAYERDRIVER_ADD_TO_NOT_BUILT)


###############################################################################
# Write a report on the driver configuration
MACRO (WRITE_DRIVER_REPORT)
    MESSAGE (STATUS "")
    MESSAGE (STATUS "===== Drivers =====")
    MESSAGE (STATUS "The following drivers will be built:")
    SET (_sortedDriverNames ${PLAYER_BUILT_DRIVERS})
    LIST (SORT _sortedDriverNames)
    FOREACH (_driverName ${_sortedDriverNames})
        MESSAGE (STATUS "${_driverName}")
    ENDFOREACH (_driverName)
    MESSAGE (STATUS "")

    MESSAGE (STATUS "The following drivers will not be built:")
    SET (_sortedDriverNames ${PLAYER_NOT_BUILT_DRIVERS})
    LIST (SORT _sortedDriverNames)
    FOREACH (_driverName ${_sortedDriverNames})
        GET_FROM_GLOBAL_MAP (_reason PLAYER_NOT_BUILT_REASONS ${_driverName})
        MESSAGE (STATUS "${_driverName} - ${_reason}")
    ENDFOREACH (_driverName)

    IF (PLAYERDRIVER_DEFINES)
        MESSAGE (STATUS "")
        MESSAGE (STATUS "Driver defines:")
        FOREACH (_define ${PLAYERDRIVER_DEFINES})
            IF (${_define})
                SET (_value ${${_define}})
            ELSE (${_define})
                SET (_value "No value/Off")
            ENDIF (${_define})
            MESSAGE (STATUS "${_define} - ${_value}")
        ENDFOREACH (_define ${PLAYERDRIVER_DEFINES})
    ENDIF (PLAYERDRIVER_DEFINES)
    MESSAGE (STATUS "===================")
    MESSAGE (STATUS "")
ENDMACRO (WRITE_DRIVER_REPORT)

###############################################################################
# Reset lists of drivers to be built/not built
MACRO (PLAYERDRIVER_RESET_LISTS)
    SET (PLAYER_BUILT_DRIVERS "" CACHE INTERNAL ${PLAYER_BUILT_DRIVERS_DESC} FORCE)

    SET (PLAYER_DRIVERSLIB_SOURCES_MAP "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_SOURCES_MAP_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_INCLUDEDIRS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_LIBDIRS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LIBDIRS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_LINKLIBS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LINKLIBS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_LINKFLAGS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_CFLAGS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_CFLAGS_DESC} FORCE)

    SET (PLAYER_NOT_BUILT_DRIVERS "" CACHE INTERNAL ${PLAYER_NOT_BUILT_DRIVERS_DESC} FORCE)
    SET (PLAYER_NOT_BUILT_REASONS "" CACHE INTERNAL ${PLAYER_NOT_BUILT_REASONS_DESC} FORCE)

    SET (PLAYERDRIVER_DEFINES "" CACHE INTERNAL ${PLAYERDRIVER_DEFINES_DESC} FORCE)
ENDMACRO (PLAYERDRIVER_RESET_LISTS)
