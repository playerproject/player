IF (HAVE_STL)
    PLAYERDRIVER_OPTION (mbasedriver build_mbasedriver ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (mbasedriver build_mbasedriver OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_REJECT_OS (mbasedriver build_mbasedriver PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (mbasedriver build_mbasedriver SOURCES mbasedriver.cc motorpacket.cc packet.cc robot_params.cc)

