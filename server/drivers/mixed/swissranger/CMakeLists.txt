PLAYERDRIVER_OPTION (swissranger build_swissranger ON)
PLAYERDRIVER_REQUIRE_HEADER_CPP (swissranger build_swissranger libMesaSR.h)
PLAYERDRIVER_ADD_DRIVER (swissranger build_swissranger LINKFLAGS "-lmesasr" SOURCES swissranger.cc)

