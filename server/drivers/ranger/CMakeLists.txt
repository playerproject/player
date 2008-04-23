PLAYERDRIVER_OPTION (gbxsickacfr build_gbxsickacfr ON)
PLAYERDRIVER_REQUIRE_PKG (gbxsickacfr build_gbxsickacfr GbxSickAcfr gbxsickacfr_includeDir
    gbxsickacfr_libDir gbxsickacfr_linkFlags gbxsickacfr_cFlags)
PLAYERDRIVER_ADD_DRIVER (gbxsickacfr build_gbxsickacfr "${gbxsickacfr_includeDir}"
    "${gbxsickacfr_libDir}" "${gbxsickacfr_linkFlags}" "${gbxsickacfr_cFlags}"
    gbxsickacfr.cc)

PLAYERDRIVER_OPTION (urg_nz build_urg_nz ON)
PLAYERDRIVER_REQUIRE_PKG (urg_nz build_urg_nz urg_nz urg_nz_includeDir urg_nz_libDir
    urg_nz_linkFlags urg_nz_cFlags)
PLAYERDRIVER_ADD_DRIVER (urg_nz build_urg_nz "${urg_nz_includeDir}" "${urg_nz_libDir}"
    "${urg_nz_linkFlags}" "${urg_nz_cFlags}"
    urg_nz.cc)

PLAYERDRIVER_OPTION (lasertoranger build_lasertoranger ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (lasertoranger build_lasertoranger toranger.cc lasertoranger.cc)

PLAYERDRIVER_OPTION (sonartoranger build_sonartoranger ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (sonartoranger build_sonartoranger toranger.cc sonartoranger.cc)