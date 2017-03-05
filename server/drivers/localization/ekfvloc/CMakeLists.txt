PLAYERDRIVER_OPTION(ekfvloc build_ekfvloc ON)
PLAYERDRIVER_REQUIRE_PKG (ekfvloc build_ekfvloc gsl
    gsl_includeDirs gsl_libDirs gsl_linkLibs
    gsl_linkFlags gsl_cFlags)
PLAYERDRIVER_REQUIRE_PKG(ekfvloc build_ekfvloc eigen3 
    eigen_includeDirs eigen_libDirs eigen_linkLibs
    eigen_linkFlags eigen_cFlags)


IF (NOT HAVE_STL)
	PLAYERDRIVER_OPTION (ekfvloc build_ekfvloc OFF "STL not found")
ELSE (NOT HAVE_STL)
	PLAYERDRIVER_OPTION (ekfvloc build_ekfvloc ON)
ENDIF (NOT HAVE_STL)

PLAYERDRIVER_REQUIRE_OS(ekfvloc build_ekfvloc UNIX)
PLAYERDRIVER_ADD_DRIVER (ekfvloc build_ekfvloc
    INCLUDEDIRS ${gsl_includeDirs} ${eigen_includeDirs}
    LIBDIRS ${gsl_libDirs} ${eigen_libDirs}
    LINKLIBS ${gsl_linkLibs} ${eigen_linkLibs}
    LINKFLAGS ${gsl_linkFlags} ${eigen_linkFlags}
    CFLAGS ${gsl_cFlags} ${eigen_cFlags}
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
