PLAYERDRIVER_OPTION (upcbarcode build_upcbarcode ON)
PLAYERDRIVER_REQUIRE_PKG (upcbarcode build_upcbarcode opencv
    upcbarcode_includeDirs upcbarcode_libDirs upcbarcode_linkLibs upcbarcode_linkFlags upcbarcode_cFlags)
PLAYERDRIVER_ADD_DRIVER (upcbarcode build_upcbarcode
    INCLUDEDIRS ${upcbarcode_includeDirs} LIBDIRS ${upcbarcode_libDirs} LINKLIBS ${upcbarcode_linkLibs}
    LINKFLAGS ${upcbarcode_linkFlags} CFLAGS ${upcbarcode_cFlags}
    SOURCES upcbarcode.cc)
