PLAYERDRIVER_OPTION (imageseq build_imageseq ON)
PLAYERDRIVER_REQUIRE_PKG (imageseq build_imageseq opencv
    imageseq_includeDir imageseq_libDir imageseq_linkFlags imageseq_cFlags)
PLAYERDRIVER_ADD_DRIVER (imageseq build_imageseq
    INCLUDEDIRS "${imageseq_includeDir}" LIBDIRS "${imageseq_libDir}"
    LINKFLAGS "${imageseq_linkFlags}" CFLAGS "${imageseq_cFlags}"
    SOURCES imageseq.cc)
