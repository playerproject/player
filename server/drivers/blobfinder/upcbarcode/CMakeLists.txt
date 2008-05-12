PLAYERDRIVER_OPTION (upcbarcode build_upcbarcode ON)
PLAYERDRIVER_REQUIRE_PKG (upcbarcode build_upcbarcode opencv
    upcbarcode_includeDir upcbarcode_libDir upcbarcode_linkFlags upcbarcode_cFlags)
PLAYERDRIVER_ADD_DRIVER (upcbarcode build_upcbarcode
    INCLUDEDIRS "${upcbarcode_includeDir}" LIBDIRS "${upcbarcode_libDir}"
    LINKFLAGS "${upcbarcode_linkFlags}" CFLAGS "${upcbarcode_cFlags}"
    SOURCES upcbarcode.cc)