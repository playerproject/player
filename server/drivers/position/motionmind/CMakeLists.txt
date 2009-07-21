SET (motionmindSrcs motionmind.cc)
PLAYERDRIVER_OPTION (motionmind build_motionmind ON)
PLAYERDRIVER_ADD_DRIVER (motionmind build_motionmind SOURCES ${motionmindSrcs})
