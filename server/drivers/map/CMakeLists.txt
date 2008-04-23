PLAYERDRIVER_OPTION (mapfile build_mapfile ON)
PLAYERDRIVER_REQUIRE_PKG (mapfile build_mapfile gdk-pixbuf-2.0
    mapfile_includeDir mapfile_libDir mapfile_linkFlags mapfile_cFlags)
PLAYERDRIVER_ADD_DRIVER (mapfile build_mapfile
    "${mapfile_includeDir}" "${mapfile_libDir}" "${mapfile_linkFlags}" "${mapfile_cFlags}"
    mapfile.cc)

PLAYERDRIVER_OPTION (mapcspace build_mapcspace ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (mapcspace build_mapcspace maptransform.cc mapcspace.cc)

PLAYERDRIVER_OPTION (mapscale build_mapscale ON)
PLAYERDRIVER_REQUIRE_PKG (mapscale build_mapscale gdk-pixbuf-2.0
    mapfile_includeDir mapfile_libDir mapfile_linkFlags mapfile_cFlags)
PLAYERDRIVER_ADD_DRIVER (mapscale build_mapscale
    "${mapfile_includeDir}" "${mapfile_libDir}" "${mapfile_linkFlags}" "${mapfile_cFlags}"
        maptransform.cc mapscale.cc)

PLAYERDRIVER_OPTION (vmapfile build_vmapfile ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (vmapfile build_vmapfile vmapfile.cc)