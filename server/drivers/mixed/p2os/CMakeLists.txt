PLAYERDRIVER_OPTION (p2os build_p2os ON)
PLAYERDRIVER_REJECT_OS (p2os build_p2os PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (p2os build_p2os SOURCES p2os.cc packet.cc robot_params.cc sip.cc kinecalc.cc)
