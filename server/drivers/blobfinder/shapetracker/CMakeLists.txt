PLAYERDRIVER_OPTION (shapetracker build_shapetracker ON)
PLAYERDRIVER_REQUIRE_PKG (shapetracker build_shapetracker opencv
    shapetracker_includeDir shapetracker_libDir shapetracker_linkFlags shapetracker_cFlags)
PLAYERDRIVER_ADD_DRIVER (shapetracker build_shapetracker
    INCLUDEDIRS "${shapetracker_includeDir}" LIBDIRS "${shapetracker_libDir}"
    LINKFLAGS "${shapetracker_linkFlags}" CFLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../.. ${shapetracker_cFlags}"
    SOURCES shapetracker.cc)
