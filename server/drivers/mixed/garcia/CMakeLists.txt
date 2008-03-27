PLAYERDRIVER_OPTION (garcia build_garcia ON)
PLAYERDRIVER_REQUIRE_PKG (garcia build_garcia libgarcia
    garcia_includeDir garcia_libDir garcia_linkFlags garcia_cFlags)
PLAYERDRIVER_ADD_DRIVER (garcia build_garcia
    "${garcia_includeDir}" "${garcia_libDir}" "${garcia_linkFlags}" "${garcia_cFlags}"
    garcia_mixed.cc)