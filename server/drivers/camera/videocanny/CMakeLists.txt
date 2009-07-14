PLAYERDRIVER_OPTION (videocanny build_videocanny ON)
PLAYERDRIVER_REQUIRE_PKG (videocanny build_videocanny opencv
    videocanny_includeDirs videocanny_libDirs videocanny_linkLibs videocanny_linkFlags videocanny_cFlags)
PLAYERDRIVER_ADD_DRIVER (videocanny build_videocanny
    INCLUDEDIRS ${videocanny_includeDirs} LIBDIRS ${videocanny_libDirs} LINKLIBS ${videocanny_linkLibs}
    LINKFLAGS ${videocanny_linkFlags} CFLAGS ${videocanny_cFlags}
    SOURCES videocanny.cc)
