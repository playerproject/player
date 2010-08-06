PLAYERDRIVER_OPTION (powershot build_powershot ON)
PLAYERDRIVER_REJECT_OS (powershot build_powershot PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (powershot build_powershot libptp2/ptp.h)
PLAYERDRIVER_REQUIRE_PKG (powershot build_powershot libexif libexif_includeDirs
    libexif_libDirs libexif_linkLibs libexif_linkFlags libexif_cFlags)
PLAYERDRIVER_REQUIRE_PKG (powershot build_powershot libusb libusb_includeDirs
    libusb_libDirs libusb_linkLibs libusb_linkFlags libusb_cFlags)
PLAYERDRIVER_ADD_DRIVER (powershot build_powershot
    INCLUDEDIRS ${libexif_includeDirs} ${libusb_includeDirs}
    LIBDIRS ${libexif_libDirs} ${libusb_libDirs}
    LINKLIBS ${libexif_linkLibs} ${libusb_linkLibs}
    LINKFLAGS ${libexif_linkFlags} ${libusb_linkFlags} -lptp2
    CFLAGS ${libexif_cFlags} ${libusb_cFlags}
    SOURCES powershot.cc)
