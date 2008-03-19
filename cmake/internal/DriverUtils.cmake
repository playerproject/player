# Macros for managing driver compiling

# Global maps are used to store the build info because we need the variables to be available
# in the parent. This has the unfortunate side-effect that it's also available everywhere
# due to being in the cache. The alternative is to make all driver CMakeLists.txt files
# INCLUDEd at the server/ level, but this would open up the worse possbility of
# cross-directory bugs between those files, which is risky when we're having driver authors
# maintain their own.

INCLUDE (${PLAYER_CMAKE_DIR}/internal/Utils.cmake)

SET (PLAYER_BUILT_DRIVERS_DESC "List of drivers that will be built")
SET (PLAYER_DRIVERSLIB_SOURCES_MAP_DESC "Source files for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC "Include dirs for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_LIBDIRS_DESC "Lib dirs for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_LINKFLAGS_DESC "Link flags for libplayerdrivers")
SET (PLAYER_DRIVERSLIB_CFLAGS_DESC "Compile flags for libplayerdrivers")
SET (PLAYER_NOT_BUILT_DRIVERS_DESC "List of drivers that will not be built")
SET (PLAYER_NOT_BUILT_REASONS_DESC "Map of reasons drivers are not being built")
SET (PLAYERDRIVER_DEFINES_DESC "List of defines for driver_config.h")

###############################################################################
# PLAYERDRIVER_ADD_DRIVER (name cumulativeVar includeDir libDir linkFlags cFlags)
# Add a driver to the list of drivers to be built or not built.
# Only call this once you have determined the final value of cumulativeVar.
# includeDir, libDir, linkFlags, and cFlags must be passed by reference.
# Don't forget to pass in the source files.
MACRO (PLAYERDRIVER_ADD_DRIVER _name _cumulativeVar _includeDir _libDir _linkFlags _cFlags)
#     MESSAGE (STATUS "Got arguments ${ARGV}")
    IF (${_cumulativeVar})
        # Add this driver's list of sources to the list of sources for libplayerdrivers
        PLAYERDRIVER_ADD_TO_BUILT (${_name} "${_includeDir}" "${_libDir}" "${_linkFlags}" "${_cFlags}" ${ARGN})
    ENDIF (${_cumulativeVar})
ENDMACRO (PLAYERDRIVER_ADD_DRIVER name cumulativeVar includeDir libDir linkFlags cFlags)


###############################################################################
# PLAYERDRIVER_ADD_DRIVER_SIMPLE (name cumulativeVar)
# Convenience wrapper for PLAYERDRIVER_ADD_DRIVER that doesn't require specifying
# directories and flags.
MACRO (PLAYERDRIVER_ADD_DRIVER_SIMPLE _name _cumulativeVar)
    IF (${_cumulativeVar})
        # Add this driver's list of sources to the list of sources for libplayerdrivers
        PLAYERDRIVER_ADD_TO_BUILT (${_name} "" "" "" "" ${ARGN})
    ENDIF (${_cumulativeVar})
ENDMACRO (PLAYERDRIVER_ADD_DRIVER_SIMPLE name cumulativeVar)


###############################################################################
# PLAYERDRIVER_OPTION (name cumulativeVar defaultValue desc [reason])
# Add an option to enable/disable a driver.
#
# name:             Driver name.
# cumulativeVar:    The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# defaultValue:     Set to ON or OFF depending on if the driver should be built
#                   by default.
# reason:           Give a reason for disabling to the user. If empty, a
#                   standard reason will be given.
MACRO (PLAYERDRIVER_OPTION _name _cumulativeVar _defaultValue)
    # Check if the disabled reason has been customised
    IF (${ARGC} GREATER 3)
        SET (_reason ${ARGV3})
    ELSE (${ARGC} GREATER 3)
        SET (_reason "Disabled by default")
    ENDIF (${ARGC} GREATER 3)
    # Add an option for it.
    PLAYERDRIVER_ADD_DRIVEROPTION (${_name} ${_defaultValue} FALSE)

    # Check if it's been disabled, if it has set cumulativeVar and
    # add it to the list of reasons
    IF (${_cumulativeVar} AND NOT ${_optionName})
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "${_reason}")
    ENDIF (${_cumulativeVar} AND NOT ${_optionName})
ENDMACRO (PLAYERDRIVER_OPTION _name _cumulativeVar _defaultValue)


###############################################################################
# PLAYERDRIVER_REQUIRE_PKG (_name _cumulativeVar _package _includeDir _libDir _linkFlags _cFlags)
# Check if a required package is available.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _package:         Name of the package config file to look for.
INCLUDE (UsePkgConfig)
MACRO (PLAYERDRIVER_REQUIRE_PKG _name _cumulativeVar _package _includeDir _libDir _linkFlags _cFlags)
    # Look for the package using pkg-config
    PKGCONFIG (${_package} ${_includeDir} ${_libDir} ${_linkFlags} ${_cFlags})
    SET (foundPackage FALSE)    # Make sure foundPackage isn't TRUE from a previous use of this macro
    IF (${_includeDir} OR ${_libDir} OR ${_linkFlags} OR ${_cFlags})
        SET (foundPackage TRUE)
    ENDIF (${_includeDir} OR ${_libDir} OR ${_linkFlags} OR ${_cFlags})
    # If not found, disable this driver
    # Derefernce cumulativeVar only once because IF will dereference the variable name stored inside itself
    IF (${_cumulativeVar} AND foundPackage)
        # Driver will be built and package found - add a #define
        STRING (TOUPPER ${_package} packageNameUpper)
        # Append to a list instead of setting an option so we can auto-generate driver_config.h
        APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_PKG_${_packageNameUpper}")
    ELSEIF (${_cumulativeVar})
        # Case where cumulativeVar is set but package wasn't found - don't build
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find package ${_package}")
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF TRUE)
    ENDIF (${_cumulativeVar} AND foundPackage)
ENDMACRO (PLAYERDRIVER_REQUIRE_PKG _name _cumulativeVar _package _includeDir _libDir _linkFlags _cFlags)


###############################################################################
# PLAYERDRIVER_REQUIRE_HEADER (_name _cumulativeVar _header)
# Check if a required header file is available.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _header:          Name of the header file to look for.
INCLUDE (CheckIncludeFiles)
MACRO (PLAYERDRIVER_REQUIRE_HEADER _name _cumulativeVar _header)
    STRING (TOUPPER ${_header} _headerUpper)
    STRING (REGEX REPLACE "[./\\]" "_" _headerUpper "${_headerUpper}")
    SET (resultVar "HAVE_HDR_${_headerUpper}")
    CHECK_INCLUDE_FILES (${_header} ${resultVar})
    # If not found, disable this driver
    # Dereference cumulativeVar only once because IF will dereference the variable name stored inside itself
    IF (${_cumulativeVar} AND ${resultVar})
        # Driver will be built and header found - add a #define
        # Append to a list instead of setting an option so we can auto-generate driver_config.h
        APPEND_TO_CACHED_LIST (PLAYERDRIVER_HAVE_DEFINES ${PLAYERDRIVER_HAVE_DEFINES_DESC} "HAVE_HDR_${_headerUpper}")
    ELSEIF (${_cumulativeVar})
        # Case where cumulativeVar is set but header wasn't found - don't build
        SET (${_cumulativeVar} FALSE)
        PLAYERDRIVER_ADD_TO_NOT_BUILT (${_name} "Could not find header ${_header}")
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF TRUE)
    ENDIF (${_cumulativeVar} AND ${resultVar})
ENDMACRO (PLAYERDRIVER_REQUIRE_HEADER _name _cumulativeVar _header)


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
    set (foundFunction)
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
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF TRUE)
    ENDIF (${_cumulativeVar} AND foundFunction)
ENDMACRO (PLAYERDRIVER_REQUIRE_FUNCTION _name _cumulativeVar _function)


###############################################################################
# PLAYERDRIVER_REQUIRE_LIB (_name _cumulativeVar _library _function _path)
# Check if a required package is available.
#
# _name:            Driver name.
# _cumulativeVar:   The option used in the calling CMakeLists.txt to check if
#                   the driver has been enabled.
# _library:         Name of the library to look for.
# _function:        Function to check the library with.
# _path:            Location where the library is expected to be.
INCLUDE (CheckLibraryExists)
MACRO (PLAYERDRIVER_REQUIRE_LIB _name _cumulativeVar _library _function _path)
    set (foundLibrary)
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
        PLAYERDRIVER_ADD_DRIVEROPTION (${_name} OFF TRUE)
    ENDIF (${_cumulativeVar} AND foundLibrary)
ENDMACRO (PLAYERDRIVER_REQUIRE_LIB _name _cumulativeVar _library _function _path)


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
ENDMACRO (PLAYERDRIVER_ADD_DEFINEOPTION _cumulativeVar _option _defaultValue _type _desc)


###############################################################################
# INTERNAL MACROS

###############################################################################
# PLAYERDRIVER_ADD_DRIVEROPTION (name value)
# Set the option for a driver to a value. If force is true, the option
# will be forced to the value rather than the cache value being used. This
# is useful to update the ccmake GUI when an option is changed internally.
MACRO (PLAYERDRIVER_ADD_DRIVEROPTION _name _value _force)
    # Get the option name for this driver
    PLAYERDRIVER_MAKE_OPTION_NAME (optionName ${_name})
    IF (force)
        # Force the option value
        SET (${optionName} ${_value} CACHE BOOL "Build driver ${_name}" FORCE)
    ELSE (force)
        SET (${optionName} ${_value} CACHE BOOL "Build driver ${_name}")
    ENDIF (force)
ENDMACRO (PLAYERDRIVER_ADD_DRIVEROPTION name value force)


###############################################################################
# Make an option name from a driver name.
MACRO (PLAYERDRIVER_MAKE_OPTION_NAME optionName driverName)
    STRING (TOUPPER ${driverName} driverNameUpper)
    SET (optionName "ENABLE_DRIVER_${driverNameUpper}")
ENDMACRO (PLAYERDRIVER_MAKE_OPTION_NAME optionName driverName)


###############################################################################
# Add a driver to the list of drivers to be built.
MACRO (PLAYERDRIVER_ADD_TO_BUILT _name _includeDir _libDir _linkFlags _cFlags)
    # Source files go into a map so we can set the cflags on them later on
    SET (tempList)
    FOREACH (sourceFile ${ARGN})
        LIST (APPEND tempList "${CMAKE_CURRENT_SOURCE_DIR}/${sourceFile}")
    ENDFOREACH (sourceFile ${ARGN})
    INSERT_INTO_GLOBAL_MAP (PLAYER_DRIVERSLIB_SOURCES_MAP ${_name} "${tempList}")

    # Append the various build options
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_INCLUDEDIRS ${PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC} ${_includeDir})
    APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LIBDIRS ${PLAYER_DRIVERSLIB_LIBDIRS_DESC} ${_libDir})
#     APPEND_TO_CACHED_LIST (PLAYER_DRIVERSLIB_LINKFLAGS ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} ${_linkFlags})
    SET (PLAYER_DRIVERSLIB_LINKFLAGS "${PLAYER_DRIVERSLIB_LINKFLAGS} ${_linkFlags}" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} FORCE)
    # C flags go into a map indexed by name because they're driver-specific
    INSERT_INTO_GLOBAL_MAP (PLAYER_DRIVERSLIB_CFLAGS ${_name} "${_cFlags}")

    # Set this driver to be built
    APPEND_TO_CACHED_LIST (PLAYER_BUILT_DRIVERS ${PLAYER_BUILT_DRIVERS_DESC} ${_name})
ENDMACRO (PLAYERDRIVER_ADD_TO_BUILT name reason)


###############################################################################
# Add a driver to the list of drivers not to be built.
MACRO (PLAYERDRIVER_ADD_TO_NOT_BUILT name reason)
    APPEND_TO_CACHED_LIST (PLAYER_NOT_BUILT_DRIVERS ${PLAYER_NOT_BUILT_DRIVERS_DESC} ${name})
    INSERT_INTO_GLOBAL_MAP (PLAYER_NOT_BUILT_REASONS ${name} ${reason})
ENDMACRO (PLAYERDRIVER_ADD_TO_NOT_BUILT name reason)


###############################################################################
# Write a report on the driver configuration
MACRO (WRITE_DRIVER_REPORT)
    MESSAGE (STATUS "")
    MESSAGE (STATUS "===== Drivers =====")
    MESSAGE (STATUS "The following drivers will be built:")
    FOREACH (_driverName ${PLAYER_BUILT_DRIVERS})
        MESSAGE (STATUS "${_driverName}")
    ENDFOREACH (_driverName)
    MESSAGE (STATUS "")

    MESSAGE (STATUS "The following drivers will not be built:")
    FOREACH (_driverName ${PLAYER_NOT_BUILT_DRIVERS})
        GET_FROM_GLOBAL_MAP (_reason PLAYER_NOT_BUILT_REASONS ${_driverName})
        MESSAGE (STATUS "${_driverName} - ${_reason}")
    ENDFOREACH (_driverName)
    MESSAGE (STATUS "")

    IF (PLAYERDRIVER_DEFINES)
        MESSAGE (STATUS "Driver defines:")
        FOREACH (_define ${PLAYERDRIVER_DEFINES})
            IF (${_define})
                SET (_value ${${_define}})
            ELSE (${_define})
                SET (_value "No value/Off")
            ENDIF (${_define})
            MESSAGE (STATUS "${_define} - ${_value}")
        ENDFOREACH (_define ${PLAYERDRIVER_DEFINES})
        MESSAGE (STATUS "")
    ENDIF (PLAYERDRIVER_DEFINES)
ENDMACRO (WRITE_DRIVER_REPORT)

###############################################################################
# Reset lists of drivers to be built/not built
MACRO (PLAYERDRIVER_RESET_LISTS)
    SET (PLAYER_BUILT_DRIVERS "" CACHE INTERNAL ${PLAYER_BUILT_DRIVERS_DESC} FORCE)

    SET (PLAYER_DRIVERSLIB_SOURCES_MAP "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_SOURCES_MAP_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_INCLUDEDIRS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_INCLUDEDIRS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_LIBDIRS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LIBDIRS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_LINKFLAGS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_LINKFLAGS_DESC} FORCE)
    SET (PLAYER_DRIVERSLIB_CFLAGS "" CACHE INTERNAL ${PLAYER_DRIVERSLIB_CFLAGS_DESC} FORCE)

    SET (PLAYER_NOT_BUILT_DRIVERS "" CACHE INTERNAL ${PLAYER_NOT_BUILT_DRIVERS_DESC} FORCE)
    SET (PLAYER_NOT_BUILT_REASONS "" CACHE INTERNAL ${PLAYER_NOT_BUILT_REASONS_DESC} FORCE)

    SET (PLAYERDRIVER_DEFINES "" CACHE INTERNAL ${PLAYERDRIVER_DEFINES_DESC} FORCE)
ENDMACRO (PLAYERDRIVER_RESET_LISTS)