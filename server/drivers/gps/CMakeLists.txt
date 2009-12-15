PLAYERDRIVER_OPTION (garminnmea build_garminnmea ON)
PLAYERDRIVER_REJECT_OS (garminnmea build_garminnmea PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (garminnmea build_garminnmea SOURCES garminnmea.cc)

PLAYERDRIVER_OPTION (gbxgarminacfr build_gbxgarminacfr ON)
PLAYERDRIVER_REQUIRE_PKG (gbxgarminacfr build_gbxgarminacfr GbxGarminAcfr
    gbxgarminacfr_includeDirs gbxgarminacfr_libDirs gbxgarminacfr_linkLibs
    gbxgarminacfr_linkFlags gbxgarminacfr_cFlags =1.0.0)
PLAYERDRIVER_ADD_DRIVER (gbxgarminacfr build_gbxgarminacfr
    INCLUDEDIRS ${gbxgarminacfr_includeDirs} LIBDIRS ${gbxgarminacfr_libDirs}
    LINKLIBS ${gbxgarminacfr_linkLibs} LINKFLAGS ${gbxgarminacfr_linkFlags}
    CFLAGS ${gbxgarminacfr_cFlags} SOURCES gbxgarminacfr.cc)

PLAYERDRIVER_OPTION (rt3xxx build_rt3xxx ON)
PLAYERDRIVER_REJECT_OS (rt3xxx build_rt3xxx PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (rt3xxx build_rt3xxx SOURCES rt3xxx.cc)

