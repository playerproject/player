IF (HAVE_STL)
    PLAYERDRIVER_OPTION (postgis build_postgis ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (postgis build_postgis OFF "STL not found")
ENDIF (HAVE_STL)
PLAYERDRIVER_REQUIRE_PKG (postgis build_postgis libpqxx
    postgis_includeDirs postgis_libDirs postgis_linkLibs postgis_linkFlags postgis_cFlags)
PLAYERDRIVER_ADD_DRIVER (postgis build_postgis
    INCLUDEDIRS "${postgis_includeDirs}" LIBDIRS "${postgis_libDirs}" LINKLIBS
    "${postgis_linkLibs}" LINKFLAGS "${postgis_linkFlags}"
    CFLAGS "${postgis_cFlags}" SOURCES postgis.cc dbconn.cc)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (vec2map build_vec2map ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (vec2map build_vec2map OFF "STL not found")
ENDIF (HAVE_STL)

SET(vec2map_linkFlags)

PLAYERDRIVER_ADD_DRIVER (vec2map build_vec2map LINKFLAGS "${vec2map_linkFlags}" SOURCES vec2map.cc)
