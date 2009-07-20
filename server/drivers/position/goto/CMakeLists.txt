PLAYERDRIVER_OPTION (goto build_goto ON)
# This driver uses GlobalTime from libplayercore, which currently doesn't
# get exported correctly.
PLAYERDRIVER_REJECT_OS (goto build_goto PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (goto build_goto SOURCES goto.cc)
