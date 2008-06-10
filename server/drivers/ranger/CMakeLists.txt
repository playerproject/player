PLAYERDRIVER_OPTION (gbxsickacfr build_gbxsickacfr ON)
PLAYERDRIVER_REQUIRE_PKG (gbxsickacfr build_gbxsickacfr GbxSickAcfr gbxsickacfr_includeDir
    gbxsickacfr_libDir gbxsickacfr_linkFlags gbxsickacfr_cFlags)
PLAYERDRIVER_ADD_DRIVER (gbxsickacfr build_gbxsickacfr
    INCLUDEDIRS "${gbxsickacfr_includeDir}" LIBDIRS "${gbxsickacfr_libDir}"
    LINKFLAGS "${gbxsickacfr_linkFlags}" CFLAGS "${gbxsickacfr_cFlags}"
    SOURCES gbxsickacfr.cc)

PLAYERDRIVER_OPTION (urg_nz build_urg_nz ON)
PLAYERDRIVER_REQUIRE_PKG (urg_nz build_urg_nz urg_nz urg_nz_includeDir urg_nz_libDir
    urg_nz_linkFlags urg_nz_cFlags)
PLAYERDRIVER_REQUIRE_PKG (urg_nz build_urg_nz flexiport flexiport_includeDir flexiport_libDir
    flexiport_linkFlags flexiport_cFlags)
PLAYERDRIVER_ADD_DRIVER (urg_nz build_urg_nz
    INCLUDEDIRS "${urg_nz_includeDir} ${flexiport_includeDir}"
    LIBDIRS "${urg_nz_libDir} ${flexiport_libDir}"
    LINKFLAGS "${urg_nz_linkFlags} ${flexiport_linkFlags}"
    CFLAGS "${urg_nz_cFlags} ${flexiport_cFlags}"
    SOURCES urg_nz.cc)

PLAYERDRIVER_OPTION (lasertoranger build_lasertoranger ON)
PLAYERDRIVER_ADD_DRIVER (lasertoranger build_lasertoranger SOURCES toranger.cc lasertoranger.cc)

PLAYERDRIVER_OPTION (sonartoranger build_sonartoranger ON)
PLAYERDRIVER_ADD_DRIVER (sonartoranger build_sonartoranger SOURCES toranger.cc sonartoranger.cc)
