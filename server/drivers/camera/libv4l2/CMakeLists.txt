PLAYERDRIVER_OPTION (libv4l2 build_libv4l2 ON)
PLAYERDRIVER_REJECT_OS (libv4l2 build_libv4l2 PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (libv4l2 build_libv4l2 linux/videodev2.h sys/types.h)
PLAYERDRIVER_REQUIRE_PKG (libv4l2 build_libv4l2 libv4l2 libv4l2_includeDirs
    libv4l2_libDirs libv4l2_linkLibs libv4l2_linkFlags libv4l2_cFlags)
PLAYERDRIVER_REQUIRE_PKG (libv4l2 build_libv4l2 libv4lconvert libv4lconvert_includeDirs
    libv4lconvert_libDirs libv4lconvert_linkLibs libv4lconvert_linkFlags libv4lconvert_cFlags)
PLAYERDRIVER_ADD_DRIVER (libv4l2 build_libv4l2
    INCLUDEDIRS ${libv4l2_includeDirs} ${libv4lconvert_includeDirs}
    LIBDIRS ${libv4l2_libDirs} ${libv4lconvert_libDirs}
    LINKLIBS ${libv4l2_linkLibs} ${libv4lconvert_linkLibs}
    LINKFLAGS ${libv4l2_linkFlags} ${libv4lconvert_linkFlags}
    CFLAGS ${libv4l2_cFlags} ${libv4lconvert_cFlags}
    SOURCES libv4l2.cc)
