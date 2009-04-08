IF (HAVE_STL)
    PLAYERDRIVER_OPTION (postgis build_postgis ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (postgis build_postgis OFF "STL not found")
ENDIF (HAVE_STL)
PLAYERDRIVER_REQUIRE_HEADER (postgis build_postgis geos_c.h)
PLAYERDRIVER_REQUIRE_PKG (postgis build_postgis libpqxx
    postgis_includeDirs postgis_libDirs postgis_linkLibs postgis_linkFlags postgis_cFlags)
PLAYERDRIVER_ADD_DRIVER (postgis build_postgis
    INCLUDEDIRS "${postgis_includeDirs}" LIBDIRS "${postgis_libDirs}" LINKLIBS
    "${postgis_linkLibs}" LINKFLAGS "-lgeos ${postgis_linkFlags}"
    CFLAGS "${postgis_cFlags}" SOURCES postgis.cc dbconn.cc)

IF (HAVE_GEOS)
    IF (HAVE_STL)
        PLAYERDRIVER_OPTION (vec2map build_vec2map ON)
    ELSE (HAVE_STL)
        PLAYERDRIVER_OPTION (vec2map build_vec2map OFF "STL not found")
    ENDIF (HAVE_STL)
ELSE (HAVE_GEOS)
    PLAYERDRIVER_OPTION (vec2map build_vec2map OFF "Geos not found")
ENDIF (HAVE_GEOS)
PLAYERDRIVER_ADD_DRIVER (vec2map build_vec2map LINKFLAGS "-lgeos -lgeos_c" SOURCES vec2map.cc)
