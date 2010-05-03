PLAYERDRIVER_OPTION (cameragst build_cameragst ON)
PLAYERDRIVER_REQUIRE_PKG (cameragst build_cameragst gstreamer-base-0.10 cameragst_includeDirs
    cameragst_libDirs cameragst_linkLibs cameragst_linkFlags cameragst_cFlags)
PLAYERDRIVER_ADD_DRIVER (cameragst build_cameragst
    INCLUDEDIRS ${cameragst_includeDirs} LIBDIRS ${cameragst_libDirs} LINKLIBS ${cameragst_linkLibs}
    LINKFLAGS ${cameragst_linkFlags} CFLAGS ${cameragst_cFlags}
    SOURCES gstappsink.c cameragst.cc)
