PLAYERDRIVER_OPTION (wavefront build_wavefront ON)
PLAYERDRIVER_REQUIRE_PKG (wavefront build_wavefront gdk-pixbuf-2.0
    wavefront_includeDirs wavefront_libDirs wavefront_linkLibs
    wavefront_linkFlags wavefront_cFlags)
PLAYERDRIVER_ADD_DRIVER (wavefront build_wavefront
    INCLUDEDIRS ${wavefront_includeDirs} LIBDIRS ${wavefront_libDirs}
    LINKLIBS ${wavefront_linkLibs} LINKFLAGS ${wavefront_linkFlags}
    CFLAGS ${wavefront_cFlags}
    SOURCES plan.c plan_plan.c plan_waypoint.c wavefront.cc heap.c plan_control.c)

# Also build and install standalone non-Player lib
IF (NOT HAVE_GETTIMEOFDAY)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
ENDIF (NOT HAVE_GETTIMEOFDAY)
PLAYER_ADD_LIBRARY (wavefront_standalone plan.c plan_plan.c plan_waypoint.c heap.c plan_control.c)
IF (NOT HAVE_GETTIMEOFDAY)
    TARGET_LINK_LIBRARIES (wavefront_standalone playerreplace)
ENDIF (NOT HAVE_GETTIMEOFDAY)
PLAYER_INSTALL_HEADERS (standalone_drivers plan.h heap.h)
