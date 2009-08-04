PLAYERDRIVER_OPTION (mbicp build_mbicp ON)
PLAYERDRIVER_REQUIRE_ENVVAR (mbicp build_mbicp MBICP_SOURCE_DIR mbicpSrcDir)
SET (mbicpSrcs mbicp_driver.cc
               ${mbicpSrcDir}/MbICP.c
               ${mbicpSrcDir}/percolate.c
               ${mbicpSrcDir}/sp_matrix.c
               ${mbicpSrcDir}/calcul.c)
PLAYERDRIVER_ADD_DRIVER (mbicp build_mbicp SOURCES ${mbicpSrcs})
