PLAYERDRIVER_OPTION (simpleshape build_simpleshape ON)
PLAYERDRIVER_REQUIRE_PKG (simpleshape build_simpleshape opencv
    simpleshape_includeDirs simpleshape_libDirs simpleshape_linkLibs simpleshape_linkFlags simpleshape_cFlags)
PLAYERDRIVER_ADD_DRIVER (simpleshape build_simpleshape
    INCLUDEDIRS ${simpleshape_includeDirs} LIBDIRS ${simpleshape_libDirs} LINKLIBS ${simpleshape_linkLibs}
    LINKFLAGS ${simpleshape_linkFlags} CFLAGS ${simpleshape_cFlags}
    SOURCES simpleshape.cc)
