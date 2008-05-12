PLAYERDRIVER_OPTION (garcia build_garcia ON)
PLAYERDRIVER_REQUIRE_PKG (garcia build_garcia libgarcia
    garcia_includeDir garcia_libDir garcia_linkFlags garcia_cFlags)
PLAYERDRIVER_ADD_DRIVER (garcia build_garcia
    INCLUDEDIRS "${garcia_includeDir}" LIBDIRS "${garcia_libDir}"
    LINKFLAGS "${garcia_linkFlags}" CFLAGS "${garcia_cFlags}"
    SOURCES garcia_mixed.cc)
