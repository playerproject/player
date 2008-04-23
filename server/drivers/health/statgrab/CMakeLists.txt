PLAYERDRIVER_OPTION (statgrab build_statgrab ON)
PLAYERDRIVER_REQUIRE_PKG (statgrab build_statgrab libstatgrab
    statgrab_includeDir statgrab_libDir statgrab_linkFlags statgrab_cFlags)
PLAYERDRIVER_ADD_DRIVER (statgrab build_statgrab
    "${statgrab_includeDir}" "${statgrab_libDir}" "${statgrab_linkFlags}" "${statgrab_cFlags}"
    statgrab_health.cpp)