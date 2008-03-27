PLAYERDRIVER_OPTION (simpleshape build_simpleshape ON)
PLAYERDRIVER_REQUIRE_PKG (simpleshape build_simpleshape opencv
    simpleshape_includeDir simpleshape_libDir simpleshape_linkFlags simpleshape_cFlags)
PLAYERDRIVER_ADD_DRIVER (simpleshape build_simpleshape
    "${simpleshape_includeDir}" "${simpleshape_libDir}" "${simpleshape_linkFlags}" "${simpleshape_cFlags}"
    simpleshape.cc)