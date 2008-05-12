SET (mricpSrcs  src/geometry2D.cpp
                src/icp.cpp
                src/lasermodel.cpp
                src/map.cpp
                src/mricp_driver.cpp
                src/nn.cpp
                src/Timer.cpp)

PLAYERDRIVER_OPTION (mricp build_mricp ON)
PLAYERDRIVER_REQUIRE_PKG (mricp build_mricp gtk+-2.0 gtk_includeDir gtk_libDir gtk_linkFlags gtk_cFlags)
PLAYERDRIVER_REQUIRE_PKG (mricp build_mricp gdk-pixbuf-2.0 gdk_pixbuf_includeDir gdk_pixbuf_libDir gdk_pixbuf_linkFlags gdk_pixbuf_cFlags)
PLAYERDRIVER_ADD_DRIVER (mricp build_mricp
    INCLUDEDIRS "${gtk_includeDir};${gdk_pixbuf_includeDir}"
    LIBDIRS "${gtk_libDir};${gdk_pixbuf_libDir}"
    LINKFLAGS "${gtk_linkFlags} ${gdk_pixbuf_linkFlags}"
    CFLAGS "-I${PROJECT_SOURCE_DIR}/server/drivers/mixed/mricp/include ${gtk_cFlags} ${gdk_pixbuf_cFlags}"
    SOURCES ${mricpSrcs})
