PLAYERDRIVER_OPTION (nxt build_nxt ON)
PLAYERDRIVER_REQUIRE_PKG (nxt build_nxt libusb-1.0
    nxt_includeDirs nxt_libDirs nxt_linkLibs
    nxt_linkFlags nxt_cFlags)

if (NOT HAVE_STL)
    PLAYERDRIVER_OPTION (nxt build_nxt OFF "STL not found")
endif(NOT HAVE_STL)

PLAYERDRIVER_ADD_DRIVER (nxt build_nxt
    INCLUDEDIRS ${nxt_includeDirs} LIBDIRS ${nxt_libDirs}
    LINKLIBS ${nxt_linkLibs} LINKFLAGS ${nxt_linkFlags}
    CFLAGS ${nxt_cFlags} 
    SOURCES     
    src/chronos.cc
    src/nxt_driver.cc
    src/nxtdc.cc
)

