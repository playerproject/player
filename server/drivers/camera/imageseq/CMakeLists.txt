PLAYERDRIVER_OPTION (imageseq build_imageseq ON)
PLAYERDRIVER_REQUIRE_PKG (imageseq build_imageseq opencv
    imageseq_includeDirs imageseq_libDirs imageseq_linkLibs imageseq_linkFlags imageseq_cFlags)
PLAYERDRIVER_ADD_DRIVER (imageseq build_imageseq
    INCLUDEDIRS ${imageseq_includeDirs} LIBDIRS ${imageseq_libDirs} LINKLIBS ${imageseq_linkLibs}
    LINKFLAGS ${imageseq_linkFlags} CFLAGS ${imageseq_cFlags}
    SOURCES imageseq.cc)
