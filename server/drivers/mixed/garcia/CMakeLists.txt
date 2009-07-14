PLAYERDRIVER_OPTION (garcia build_garcia ON)
PLAYERDRIVER_REQUIRE_PKG (garcia build_garcia libgarcia
    garcia_includeDirs garcia_libDirs garcia_linkLibs garcia_linkFlags garcia_cFlags)
PLAYERDRIVER_ADD_DRIVER (garcia build_garcia
    INCLUDEDIRS ${garcia_includeDir} LIBDIRS ${garcia_libDir} LINKLIBS ${garcia_linkLibs}
    LINKFLAGS ${garcia_linkFlags} CFLAGS ${garcia_cFlags}
    SOURCES garcia_mixed.cc)
