PLAYERDRIVER_OPTION (oceanserver build_oceanserver ON)
PLAYERDRIVER_REQUIRE_PKG (oceanserver build_oceanserver GbxSmartBatteryAcfr
    gbxsmartbatteryacfr_includeDirs gbxsmartbatteryacfr_libDirs gbxsmartbatteryacfr_linkLibs
    gbxsmartbatteryacfr_linkFlags gbxsmartbatteryacfr_cFlags =1.0.0)
PLAYERDRIVER_ADD_DRIVER (oceanserver build_oceanserver
    INCLUDEDIRS ${gbxsmartbatteryacfr_includeDirs} LIBDIRS ${gbxsmartbatteryacfr_libDirs}
    LINKLIBS ${gbxsmartbatteryacfr_linkLibs} LINKFLAGS ${gbxsmartbatteryacfr_linkFlags}
    CFLAGS ${gbxsmartbatteryacfr_cFlags} SOURCES oceanserver.cc)

