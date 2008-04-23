PLAYERDRIVER_OPTION (upcbarcode build_upcbarcode ON)
PLAYERDRIVER_REQUIRE_PKG (upcbarcode build_upcbarcode opencv
    upcbarcode_includeDir upcbarcode_libDir upcbarcode_linkFlags upcbarcode_cFlags)
PLAYERDRIVER_ADD_DRIVER (upcbarcode build_upcbarcode
    "${upcbarcode_includeDir}" "${upcbarcode_libDir}" "${upcbarcode_linkFlags}" "${upcbarcode_cFlags}"
    upcbarcode.cc)