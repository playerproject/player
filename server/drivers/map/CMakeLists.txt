PLAYERDRIVER_OPTION (mapfile build_mapfile ON)
PLAYERDRIVER_REQUIRE_PKG (mapfile build_mapfile gdk-pixbuf-2.0
    mapfile_includeDir mapfile_libDir mapfile_linkFlags mapfile_cFlags)
PLAYERDRIVER_ADD_DRIVER (mapfile build_mapfile
    INCLUDEDIRS "${mapfile_includeDir}" LIBDIRS "${mapfile_libDir}"
    LINKFLAGS "${mapfile_linkFlags}" CFLAGS "${mapfile_cFlags}"
    SOURCES mapfile.cc)

PLAYERDRIVER_OPTION (mapcspace build_mapcspace ON)
PLAYERDRIVER_ADD_DRIVER (mapcspace build_mapcspace SOURCES maptransform.cc mapcspace.cc)

PLAYERDRIVER_OPTION (mapscale build_mapscale ON)
PLAYERDRIVER_REQUIRE_PKG (mapscale build_mapscale gdk-pixbuf-2.0
    mapfile_includeDir mapfile_libDir mapfile_linkFlags mapfile_cFlags)
PLAYERDRIVER_ADD_DRIVER (mapscale build_mapscale
    INCLUDEDIRS "${mapfile_includeDir}" LIBDIRS "${mapfile_libDir}"
    LINKFLAGS "${mapfile_linkFlags}" CFLAGS "${mapfile_cFlags}"
    SOURCES maptransform.cc mapscale.cc)

PLAYERDRIVER_OPTION (vmapfile build_vmapfile ON)
PLAYERDRIVER_ADD_DRIVER (vmapfile build_vmapfile SOURCES vmapfile.cc)
