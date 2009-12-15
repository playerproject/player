PLAYERDRIVER_OPTION (service_adv_mdns build_service_adv_mdns OFF "Disabled by default")
PLAYERDRIVER_REQUIRE_PKG (service_adv_mdns build_service_adv_mdns howl
    howl__includeDirs howl_libDirs howl_linkLibs howl_linkFlags howl_cFlags ">=0.9.6")
PLAYERDRIVER_ADD_DRIVER (service_adv_mdns build_service_adv_mdns
    INCLUDEDIRS {howl__includeDirs} LIBDIRS ${howl_libDirs} LINKLIBS
    ${howl_linkLibs} LINKFLAGS ${howl_linkFlags} CFLAGS ${howl_cFlags}
    SOURCES mdns.cc)
