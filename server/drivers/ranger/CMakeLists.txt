PLAYERDRIVER_OPTION (gbxsickacfr build_gbxsickacfr ON)
PLAYERDRIVER_REQUIRE_PKG (gbxsickacfr build_gbxsickacfr GbxSickAcfr
    gbxsickacfr_includeDirs gbxsickacfr_libDirs gbxsickacfr_linkLibs
    gbxsickacfr_linkFlags gbxsickacfr_cFlags =1.0.0)
PLAYERDRIVER_ADD_DRIVER (gbxsickacfr build_gbxsickacfr
    INCLUDEDIRS ${gbxsickacfr_includeDirs} LIBDIRS ${gbxsickacfr_libDirs}
    LINKLIBS ${gbxsickacfr_linkLibs} LINKFLAGS ${gbxsickacfr_linkFlags}
    CFLAGS ${gbxsickacfr_cFlags} SOURCES gbxsickacfr.cc)

PLAYERDRIVER_OPTION (hokuyoaist build_hokuyoaist ON)
PLAYERDRIVER_REQUIRE_PKG (hokuyoaist build_hokuyoaist hokuyoaist
    hokuyoaist_includeDirs hokuyoaist_libDirs hokuyoaist_linkLibs
    hokuyoaist_linkFlags hokuyoaist_cFlags =3.0.0)
PLAYERDRIVER_ADD_DRIVER (hokuyoaist build_hokuyoaist
    INCLUDEDIRS ${hokuyoaist_includeDirs} LIBDIRS ${hokuyoaist_libDirs}
    LINKLIBS ${hokuyoaist_linkLibs} LINKFLAGS ${hokuyoaist_linkFlags}
    CFLAGS ${hokuyoaist_cFlags} SOURCES hokuyo_aist.cc)

PLAYERDRIVER_OPTION (lasertoranger build_lasertoranger ON)
PLAYERDRIVER_ADD_DRIVER (lasertoranger build_lasertoranger SOURCES toranger.cc lasertoranger.cc)

PLAYERDRIVER_OPTION (sonartoranger build_sonartoranger ON)
PLAYERDRIVER_ADD_DRIVER (sonartoranger build_sonartoranger SOURCES toranger.cc sonartoranger.cc)

PLAYERDRIVER_OPTION (rangertolaser build_rangertolaser ON)
PLAYERDRIVER_ADD_DRIVER (rangertolaser build_rangertolaser SOURCES fromranger.cc rangertolaser.cc)

PLAYERDRIVER_OPTION (rangerposeinterpolator build_rangerposeinterpolator ON)
PLAYERDRIVER_ADD_DRIVER (rangerposeinterpolator build_rangerposeinterpolator SOURCES rangerposeinterpolator.cc)
