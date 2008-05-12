PLAYERDRIVER_OPTION (artoolkitplus build_artoolkitplus ON)
PLAYERDRIVER_REQUIRE_PKG (artoolkitplus build_artoolkitplus artoolkitplus
    artoolkitplus_includeDir artoolkitplus_libDir artoolkitplus_linkFlags artoolkitplus_cFlags ">= 2.1")
PLAYERDRIVER_ADD_DRIVER (artoolkitplus build_artoolkitplus
    INCLUDEDIRS "${artoolkitplus_includeDir}" LIBDIRS "${artoolkitplus_libDir}"
    LINKFLAGS "${artoolkitplus_linkFlags}" CFLAGS "${artoolkitplus_cFlags}"
    SOURCES artoolkitplus.cc)
