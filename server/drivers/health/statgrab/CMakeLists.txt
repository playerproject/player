PLAYERDRIVER_OPTION (statgrab build_statgrab ON)
PLAYERDRIVER_REQUIRE_PKG (statgrab build_statgrab libstatgrab
    statgrab_includeDir statgrab_libDir statgrab_linkFlags statgrab_cFlags)
PLAYERDRIVER_ADD_DRIVER (statgrab build_statgrab
    INCLUDEDIRS "${statgrab_includeDir}" LIBDIRS "${statgrab_libDir}"
    LINKFLAGS "${statgrab_linkFlags}" CFLAGS "${statgrab_cFlags}"
    SOURCES statgrab_health.cpp)
