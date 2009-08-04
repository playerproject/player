PLAYERDRIVER_OPTION (nd build_nd ON)
PLAYERDRIVER_REQUIRE_ENVVAR (nd build_nd ND_SOURCE_DIR ndSrcDir)
SET (ndSrcs nd_plugin.cc
            ${ndSrcDir}/nd.cc
            ${ndSrcDir}/geometria.cc)
PLAYERDRIVER_ADD_DRIVER (nd build_nd SOURCES ${ndSrcs})
