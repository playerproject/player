PLAYERDRIVER_OPTION (cvcam build_cvcam ON)
PLAYERDRIVER_REQUIRE_PKG (cvcam build_cvcam opencv cvcam_includeDirs
    cvcam_libDirs cvcam_linkLibs cvcam_linkFlags cvcam_cFlags)
PLAYERDRIVER_ADD_DRIVER (cvcam build_cvcam
    INCLUDEDIRS ${cvcam_includeDirs} LIBDIRS ${cvcam_libDirs} LINKLIBS ${cvcam_linkLibs}
    LINKFLAGS ${cvcam_linkFlags} CFLAGS ${cvcam_cFlags}
    SOURCES cvcam.cc)
