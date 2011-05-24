PLAYERDRIVER_OPTION (kinect build_kinect ON)

PLAYERDRIVER_REQUIRE_PKG (kinect build_kinect libfreenect freenect_includeDirs
    freenect_libDirs freenect_linkLibs freenect_linkFlags freenect_cFlags)
PLAYERDRIVER_REQUIRE_PKG (kinect build_kinect libusb-1.0 libusb_includeDirs
    libusb_libDirs libusb_linkLibs libusb_linkFlags libusb_cFlags)

PLAYERDRIVER_ADD_DRIVER (kinect build_kinect
                         INCLUDEDIRS ${freenect_includeDirs} 
                         LIBDIRS ${freenect_libDirs} 
                         LINKLIBS ${freenect_linkLibs} ${libusb_linkLibs}
                         LINKFLAGS ${freenect_linkFlags} 
                         CFLAGS ${freenect_cFlags} ${libusb_cFlags}
                         SOURCES kinectdriver.cc)
