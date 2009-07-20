SET (motionmindSrcs motionmind.cc)
PLAYERDRIVER_OPTION (motionmind build_motionmind ON)
# This driver uses GlobalTime from libplayercore, which currently doesn't
# get exported correctly.
PLAYERDRIVER_REJECT_OS (motionmind build_motionmind PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (motionmind build_motionmind SOURCES ${motionmindSrcs})
