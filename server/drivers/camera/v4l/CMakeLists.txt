PLAYERDRIVER_OPTION (camerav4l build_camerav4l ON)
PLAYERDRIVER_REJECT_OS (camerav4l build_camerav4l PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (camerav4l build_camerav4l linux/videodev.h sys/types.h)
PLAYERDRIVER_ADD_DRIVER (camerav4l build_camerav4l SOURCES camerav4l.cc v4lcapture.c v4lframe.c ccvt_c1.c)
