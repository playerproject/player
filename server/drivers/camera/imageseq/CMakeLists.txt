PLAYERDRIVER_OPTION (imageseq build_imageseq ON)
PLAYERDRIVER_REQUIRE_PKG (imageseq build_imageseq opencv
    imageseq_includeDir imageseq_libDir imageseq_linkFlags imageseq_cFlags)
PLAYERDRIVER_ADD_DRIVER (imageseq build_imageseq
    "${imageseq_includeDir}" "${imageseq_libDir}" "${imageseq_linkFlags}" "${imageseq_cFlags}"
    imageseq.cc)