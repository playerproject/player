PLAYERDRIVER_OPTION (postgis build_postgis ON)
PLAYERDRIVER_REQUIRE_HEADER (postgis build_postgis geos_c.h)
PLAYERDRIVER_REQUIRE_PKG (postgis build_postgis libpqxx
    postgis_includeDir postgis_libDir postgis_linkFlags postgis_cFlags)
PLAYERDRIVER_ADD_DRIVER (postgis build_postgis
    INCLUDEDIRS "${postgis_includeDir}" LIBDIRS "${postgis_libDir}"
    LINKFLAGS "-lgeos ${postgis_linkFlags}" CFLAGS "${postgis_cFlags}"
    SOURCES postgis.cc dbconn.cc)

PLAYERDRIVER_OPTION (vec2map build_vec2map ON)
PLAYERDRIVER_REQUIRE_HEADER (vec2map build_vec2map geos_c.h)
PLAYERDRIVER_ADD_DRIVER (postgis build_postgis LINKFLAGS "-lgeos" SOURCES vec2map.cc)
