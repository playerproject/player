PLAYERDRIVER_OPTION (flexiport build_flexiport ON)
PLAYERDRIVER_REQUIRE_PKG (flexiport build_flexiport flexiport flexiport_includeDir flexiport_libDir
    flexiport_linkFlags flexiport_cFlags)
PLAYERDRIVER_ADD_DRIVER (flexiport build_flexiport INCLUDEDIRS "${flexiport_includeDir}"
    LIBDIRS "${flexiport_libDir}" LINKFLAGS "${flexiport_linkFlags}" CFLAGS "${flexiport_cFlags}"
    SOURCES flexiport.cc)

PLAYERDRIVER_OPTION (serialstream build_serialstream ON)
PLAYERDRIVER_REJECT_OS (serialstream build_serialstream PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (serialstream build_serialstream SOURCES serialstream.cc)

PLAYERDRIVER_OPTION (tcpstream build_tcpstream ON)
PLAYERDRIVER_REJECT_OS (tcpstream build_tcpstream PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (tcpstream build_tcpstream SOURCES tcpstream.cc)

