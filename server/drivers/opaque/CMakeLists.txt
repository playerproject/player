PLAYERDRIVER_OPTION (flexiport build_flexiport ON)
PLAYERDRIVER_REQUIRE_PKG (flexiport build_flexiport flexiport flexiport_includeDirs
    flexiport_libDirs flexiport_linkLibs flexiport_linkFlags flexiport_cFlags =1.0.0)
PLAYERDRIVER_ADD_DRIVER (flexiport build_flexiport
    INCLUDEDIRS ${flexiport_includeDirs} LIBDIRS ${flexiport_libDirs}
    LINKLIBS ${flexiport_linkLibs} LINKFLAGS ${flexiport_linkFlags} CFLAGS ${flexiport_cFlags}
    SOURCES flexiport.cc)

PLAYERDRIVER_OPTION (serialstream build_serialstream ON)
PLAYERDRIVER_REJECT_OS (serialstream build_serialstream PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (serialstream build_serialstream SOURCES serialstream.cc)

PLAYERDRIVER_OPTION (tcpstream build_tcpstream ON)
PLAYERDRIVER_REJECT_OS (tcpstream build_tcpstream PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (tcpstream build_tcpstream SOURCES tcpstream.cc)

