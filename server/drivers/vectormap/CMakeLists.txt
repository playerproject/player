IF (HAVE_STL)
    PLAYERDRIVER_OPTION (postgis build_postgis ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (postgis build_postgis OFF "STL not found")
ENDIF (HAVE_STL)
PLAYERDRIVER_REQUIRE_PKG (postgis build_postgis libpqxx
    postgis_includeDirs postgis_libDirs postgis_linkLibs postgis_linkFlags postgis_cFlags)
IF (PLAYER_OS_SOLARIS)
    SET (pqLinkFlag "-lpq")
ELSE (PLAYER_OS_SOLARIS)
    SET (pqLinkFlag)
ENDIF (PLAYER_OS_SOLARIS)
PLAYERDRIVER_ADD_DRIVER (postgis build_postgis
    INCLUDEDIRS ${postgis_includeDirs} LIBDIRS ${postgis_libDirs}
    LINKLIBS ${postgis_linkLibs} LINKFLAGS ${postgis_linkFlags} ${pqLinkFlag}
    CFLAGS ${postgis_cFlags} SOURCES postgis.cc dbconn.cc)

PLAYERDRIVER_OPTION (vec2map build_vec2map ON)
PLAYERDRIVER_ADD_DRIVER (vec2map build_vec2map SOURCES vec2map.cc)

PLAYERDRIVER_OPTION (robotracker build_robotracker ON)
PLAYERDRIVER_ADD_DRIVER (robotracker build_robotracker SOURCES robotracker.cc)
