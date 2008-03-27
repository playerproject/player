PLAYERDRIVER_OPTION (camerauvc build_camerauvc ON)
PLAYERDRIVER_REQUIRE_HEADER (camerauvc build_camerauvc linux/videodev2.h)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (camerauvc build_camerauvc cameraUVC.cc UvcInterface.cc)