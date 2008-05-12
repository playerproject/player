PLAYERDRIVER_OPTION (serialstream build_serialstream ON)
PLAYERDRIVER_ADD_DRIVER (serialstream build_serialstream SOURCES serialstream.cc)

PLAYERDRIVER_OPTION (tcpstream build_tcpstream ON)
PLAYERDRIVER_ADD_DRIVER (tcpstream build_tcpstream SOURCES tcpstream.cc)
