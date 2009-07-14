PLAYERDRIVER_OPTION (statgrab build_statgrab ON)
PLAYERDRIVER_REQUIRE_PKG (statgrab build_statgrab libstatgrab
    statgrab_includeDirs statgrab_libDirs statgrab_linkLibs statgrab_linkFlags statgrab_cFlags)
PLAYERDRIVER_ADD_DRIVER (statgrab build_statgrab
    INCLUDEDIRS ${statgrab_includeDirs} LIBDIRS ${statgrab_libDirs} LINKLIBS ${statgrab_linkLibs}
    LINKFLAGS ${statgrab_linkFlags} CFLAGS ${statgrab_cFlags}
    SOURCES statgrab_health.cpp)
