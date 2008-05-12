PLAYERDRIVER_OPTION (camerav4l build_camerav4l ON)
PLAYERDRIVER_REQUIRE_HEADER (camerav4l build_camerav4l linux/videodev.h)
PLAYERDRIVER_ADD_DRIVER (camerav4l build_camerav4l SOURCES camerav4l.cc v4lcapture.c v4lframe.c ccvt_c1.c)
