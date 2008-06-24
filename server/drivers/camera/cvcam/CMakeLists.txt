PLAYERDRIVER_OPTION (cvcam build_cvcam ON)
PLAYERDRIVER_REQUIRE_PKG (cvcam build_cvcam opencv cvcam_includeDir
    cvcam_libDir cvcam_linkFlags cvcam_cFlags)
PLAYERDRIVER_ADD_DRIVER (cvcam build_cvcam
    INCLUDEDIRS "${cvcam_includeDir}" LIBDIRS "${cvcam_libDir}"
    LINKFLAGS "${cvcam_linkFlags}" CFLAGS "${cvcam_cFlags}"
    SOURCES cvcam.cc)
