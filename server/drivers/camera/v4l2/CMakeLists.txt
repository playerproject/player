PLAYERDRIVER_OPTION (camerav4l2 build_camerav4l2 ON)
PLAYERDRIVER_REJECT_OS (camerav4l2 build_camerav4l2 PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (camerav4l2 build_camerav4l2 linux/videodev2.h sys/types.h)
PLAYERDRIVER_ADD_DRIVER (camerav4l2 build_camerav4l2 SOURCES bayer.c v4l2.c camerav4l2.cc)
