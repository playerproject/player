PLAYERDRIVER_OPTION (artoolkitplus build_artoolkitplus ON)
PLAYERDRIVER_REQUIRE_PKG (artoolkitplus build_artoolkitplus artoolkitplus
    artoolkitplus_includeDirs artoolkitplus_libDirs artoolkitplus_linkLibs artoolkitplus_linkFlags artoolkitplus_cFlags >=2.1)
PLAYERDRIVER_ADD_DRIVER (artoolkitplus build_artoolkitplus
    INCLUDEDIRS ${artoolkitplus_includeDirs} LIBDIRS ${artoolkitplus_libDirs} LINKLIBS ${artoolkitplus_linkLibs}
    LINKFLAGS ${artoolkitplus_linkFlags} CFLAGS ${artoolkitplus_cFlags}
    SOURCES artoolkitplus.cc)
