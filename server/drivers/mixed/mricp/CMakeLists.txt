SET (mricpSrcs  src/geometry2D.cpp
                src/icp.cpp
                src/lasermodel.cpp
                src/map.cpp
                src/mricp_driver.cpp
                src/nn.cpp
                src/Timer.cpp)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (mricp build_mricp ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (mricp build_mricp OFF "STL not found")
ENDIF (HAVE_STL)
PLAYERDRIVER_REQUIRE_PKG (mricp build_mricp gtk+-2.0 gtk_includeDirs gtk_libDirs gtk_linkLibs gtk_linkFlags gtk_cFlags)
PLAYERDRIVER_REQUIRE_PKG (mricp build_mricp gdk-pixbuf-2.0 gdk_pixbuf_includeDirs gdk_pixbuf_libDirs gdk_pixbuf_linkLibs gdk_pixbuf_linkFlags gdk_pixbuf_cFlags)
PLAYERDRIVER_ADD_DRIVER (mricp build_mricp
    INCLUDEDIRS ${gtk_includeDirs} ${gdk_pixbuf_includeDirs}
    LIBDIRS ${gtk_libDirs} ${gdk_pixbuf_libDirs}
    LINKLIBS ${gtk_linkLibs} ${gdk_pixbuf_linkLibs}
    LINKFLAGS ${gtk_linkFlags} ${gdk_pixbuf_linkFlags}
    CFLAGS -I${PROJECT_SOURCE_DIR}/server/drivers/mixed/mricp/include ${gtk_cFlags} ${gdk_pixbuf_cFlags}
    SOURCES ${mricpSrcs})
