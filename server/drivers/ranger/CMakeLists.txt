PLAYERDRIVER_OPTION (gbxsickacfr build_gbxsickacfr ON)
PLAYERDRIVER_REQUIRE_PKG (gbxsickacfr build_gbxsickacfr GbxSickAcfr gbxsickacfr_includeDir
    gbxsickacfr_libDir gbxsickacfr_linkFlags gbxsickacfr_cFlags)
PLAYERDRIVER_ADD_DRIVER (gbxsickacfr build_gbxsickacfr
    INCLUDEDIRS "${gbxsickacfr_includeDir}" LIBDIRS "${gbxsickacfr_libDir}"
    LINKFLAGS "${gbxsickacfr_linkFlags}" CFLAGS "${gbxsickacfr_cFlags}"
    SOURCES gbxsickacfr.cc)

PLAYERDRIVER_OPTION (hokuyo_aist build_hokuyo_aist ON)
PLAYERDRIVER_REQUIRE_PKG (hokuyo_aist build_hokuyo_aist hokuyo_aist hokuyo_aist_includeDir
    hokuyo_aist_libDir hokuyo_aist_linkFlags hokuyo_aist_cFlags)
PLAYERDRIVER_ADD_DRIVER (hokuyo_aist build_hokuyo_aist INCLUDEDIRS "${hokuyo_aist_includeDir}"
    LIBDIRS "${hokuyo_aist_libDir}" LINKFLAGS "${hokuyo_aist_linkFlags}"
    CFLAGS "${hokuyo_aist_cFlags}" SOURCES hokuyo_aist.cc)

PLAYERDRIVER_OPTION (lasertoranger build_lasertoranger ON)
PLAYERDRIVER_ADD_DRIVER (lasertoranger build_lasertoranger SOURCES toranger.cc lasertoranger.cc)

PLAYERDRIVER_OPTION (sonartoranger build_sonartoranger ON)
PLAYERDRIVER_ADD_DRIVER (sonartoranger build_sonartoranger SOURCES toranger.cc sonartoranger.cc)
