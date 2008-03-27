PLAYERDRIVER_OPTION (artoolkitplus build_artoolkitplus ON)
PLAYERDRIVER_REQUIRE_PKG (artoolkitplus build_artoolkitplus artoolkitplus
    artoolkitplus_includeDir artoolkitplus_libDir artoolkitplus_linkFlags artoolkitplus_cFlags ">= 2.1")
PLAYERDRIVER_ADD_DRIVER (artoolkitplus build_artoolkitplus
    "${artoolkitplus_includeDir}" "${artoolkitplus_libDir}" "${artoolkitplus_linkFlags}" "${artoolkitplus_cFlags}"
    artoolkitplus.cc)