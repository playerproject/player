PLAYERDRIVER_OPTION (roomba build_roomba ON)
PLAYERDRIVER_REJECT_OS (roomba build_roomba PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (roomba build_roomba SOURCES roomba_comms.c roomba_driver.cc)
