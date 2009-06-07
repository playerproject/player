PLAYERDRIVER_OPTION (epuck build_epuck ON)
PLAYERDRIVER_REJECT_OS (epuck build_epuck PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (epuck build_epuck SOURCES epuckCamera.cpp epuckDriver.cpp epuckInterface.cpp epuckIR.cpp epuckLEDs.cpp epuckPosition2d.cpp serialPort.cpp)
