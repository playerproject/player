PLAYERDRIVER_OPTION (wlanscan build_wlanscan ON)
PLAYERDRIVER_REJECT_OS (wlanscan build_wlanscan PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_PKG (wlanscan build_wlanscan libnl-1
    wlanscan_includeDirs wlanscan_libDirs wlanscan_linkLibs wlanscan_linkFlags wlanscan_cFlags)
PLAYERDRIVER_ADD_DRIVER (wlanscan build_wlanscan 
    INCLUDEDIRS ${wlanscan_includeDirs} LIBDIRS ${wlanscan_libDirs} LINKLIBS ${wlanscan_linkLibs}
    LINKFLAGS ${wlanscan_linkFlags} CFLAGS ${wlanscan_cFlags}
    SOURCES wlanscan.cc scan.c handler.c)
