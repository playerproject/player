PLAYERDRIVER_OPTION(ekfvloc build_ekfvloc ON)
PLAYERDRIVER_REQUIRE_PKG (ekfvloc build_ekfvloc gsl
    ekfvloc_includeDirs ekfvloc_libDirs ekfvloc_linkLibs
    ekfvloc_linkFlags ekfvloc_cFlags)


IF (NOT HAVE_STL)
	PLAYERDRIVER_OPTION (ekfvloc build_ekfvloc OFF "STL not found")
ELSE (NOT HAVE_STL)
	PLAYERDRIVER_OPTION (ekfvloc build_ekfvloc ON)
ENDIF (NOT HAVE_STL)

PLAYERDRIVER_REQUIRE_OS(ekfvloc build_ekfvloc UNIX)
PLAYERDRIVER_ADD_DRIVER (ekfvloc build_ekfvloc
    INCLUDEDIRS ${ekfvloc_includeDirs} LIBDIRS ${ekfvloc_libDirs}
    LINKLIBS ${ekfvloc_linkLibs} LINKFLAGS ${ekfvloc_linkFlags}
    CFLAGS ${ekfvloc_cFlags} 
    SOURCES
    ekfvloc_driver.cc
    feature.cc
    hregions.cc
    localize.cc
    params.cc
    robot_location.cc
    scan.cc
    segment_map.cc
    sub_opt.cc
    transf.cc
    uloc.cc
)
