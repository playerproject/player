PLAYERDRIVER_OPTION (unicapimage build_unicapimage ON)

PLAYERDRIVER_REQUIRE_PKG (unicapimage build_unicapimage libunicap unicap_includeDirs
    unicap_libDirs unicap_linkLibs unicap_linkFlags unicap_cFlags)


PLAYERDRIVER_ADD_DRIVER (unicapimage build_unicapimage
                         INCLUDEDIRS ${unicap_includeDirs} 
                         LIBDIRS ${unicap_libDirs} 
                         LINKLIBS ${unicap_linkLibs}
                         LINKFLAGS ${unicap_linkFlags} 
                         CFLAGS ${unicap_cFlags}
                         SOURCES unicapImage.cc)
