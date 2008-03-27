PLAYERDRIVER_OPTION (postgis build_postgis ON)
PLAYERDRIVER_REQUIRE_HEADER (postgis build_postgis geos_c.h)
PLAYERDRIVER_REQUIRE_PKG (postgis build_postgis libpqxx
    postgis_includeDir postgis_libDir postgis_linkFlags postgis_cFlags)
PLAYERDRIVER_ADD_DRIVER (postgis build_postgis
    "${postgis_includeDir}" "${postgis_libDir}" "-lgeos ${postgis_linkFlags}" "${postgis_cFlags}"
    postgis.cc dbconn.cc)