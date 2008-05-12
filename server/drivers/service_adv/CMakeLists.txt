PLAYERDRIVER_OPTION (service_adv_mdns build_service_adv_mdns OFF "Disabled by default")
PLAYERDRIVER_REQUIRE_PKG (service_adv_mdns build_service_adv_mdns howl
    howl__includeDir howl_libDir howl_linkFlags howl_cFlags ">= 0.9.6")
PLAYERDRIVER_ADD_DRIVER (service_adv_mdns build_service_adv_mdns
    INCLUDEDIRS "${howl__includeDir}" LIBDIRS "${howl_libDir}"
    LINKFLAGS "${howl_linkFlags}" CFLAGS "${howl_cFlags}"
    SOURCES mdns.cc)