PLAYERDRIVER_OPTION (gbxsickacfr build_gbxsickacfr ON)
PLAYERDRIVER_REQUIRE_PKG (gbxsickacfr build_gbxsickacfr GbxSickAcfr
    gbxsickacfr_includeDirs gbxsickacfr_libDirs gbxsickacfr_linkLibs
    gbxsickacfr_linkFlags gbxsickacfr_cFlags =1.0.0)
PLAYERDRIVER_ADD_DRIVER (gbxsickacfr build_gbxsickacfr
    INCLUDEDIRS ${gbxsickacfr_includeDirs} LIBDIRS ${gbxsickacfr_libDirs}
    LINKLIBS ${gbxsickacfr_linkLibs} LINKFLAGS ${gbxsickacfr_linkFlags}
    CFLAGS ${gbxsickacfr_cFlags} SOURCES gbxsickacfr.cc)

PLAYERDRIVER_OPTION (hokuyo_aist build_hokuyo_aist ON)
PLAYERDRIVER_REQUIRE_PKG (hokuyo_aist build_hokuyo_aist hokuyo_aist
    hokuyo_aist_includeDirs hokuyo_aist_libDirs hokuyo_aist_linkLibs
    hokuyo_aist_linkFlags hokuyo_aist_cFlags =1.0.0)
PLAYERDRIVER_ADD_DRIVER (hokuyo_aist build_hokuyo_aist
    INCLUDEDIRS ${hokuyo_aist_includeDirs} LIBDIRS ${hokuyo_aist_libDirs}
    LINKLIBS ${hokuyo_aist_linkLibs} LINKFLAGS ${hokuyo_aist_linkFlags}
    CFLAGS ${hokuyo_aist_cFlags} SOURCES hokuyo_aist.cc)

PLAYERDRIVER_OPTION (lasertoranger build_lasertoranger ON)
PLAYERDRIVER_ADD_DRIVER (lasertoranger build_lasertoranger SOURCES toranger.cc lasertoranger.cc)

PLAYERDRIVER_OPTION (sonartoranger build_sonartoranger ON)
PLAYERDRIVER_ADD_DRIVER (sonartoranger build_sonartoranger SOURCES toranger.cc sonartoranger.cc)

PLAYERDRIVER_OPTION (rangertolaser build_rangertolaser ON)
PLAYERDRIVER_ADD_DRIVER (rangertolaser build_rangertolaser SOURCES fromranger.cc rangertolaser.cc)

PLAYERDRIVER_OPTION (rangerposeinterpolator build_rangerposeinterpolator ON)
PLAYERDRIVER_ADD_DRIVER (rangerposeinterpolator build_rangerposeinterpolator SOURCES rangerposeinterpolator.cc)
