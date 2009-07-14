PLAYERDRIVER_OPTION (amtecm5 build_amtecm5 ON)
PLAYERDRIVER_REJECT_OS (amtecm5 build_amtecm5 PLAYER_OS_WIN)
SET (AMTECM5_DIR "" CACHE STRING "Directory containing the Amtec M5 libraries")
MARK_AS_ADVANCED (AMTECM5_DIR)

IF (NOT "${AMTECM5_DIR}" STREQUAL "")
    SET (amtecExtraIncludeDirs ${AMTECM5_DIR}/include ${AMTECM5_DIR}/Device)
ENDIF (NOT "${AMTECM5_DIR}" STREQUAL "")
SET (amtecExtraLibs "-ldevice -lntcan -lutil")
SET (amtecCFlags "-D__LINUX__ -DUNIX -DLINUX")

SET (CMAKE_REQUIRED_INCLUDES "${amtecExtraIncludeDirs}")
SET (CMAKE_REQUIRED_DEFINITIONS "${amtecCFlags}")
PLAYERDRIVER_REQUIRE_HEADER_CPP (amtecm5 build_amtecm5 Device.h)
SET (CMAKE_REQUIRED_INCLUDES)
SET (CMAKE_REQUIRED_DEFINITIONS)

PLAYERDRIVER_ADD_DRIVER (amtecm5 build_amtecm5
    INCLUDEDIRS ${amtecExtraIncludeDirs} LIBDIRS ${AMTECM5_DIR}/lib
    LINKFLAGS ${amtecExtraLibs} CFLAGS ${amtecCFlags} SOURCES amtecM5.cc)
