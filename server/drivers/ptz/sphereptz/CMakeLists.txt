PLAYERDRIVER_OPTION (sphereptz build_sphereptz ON)
PLAYERDRIVER_REJECT_OS (sphereptz build_sphereptz PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (sphereptz build_sphereptz linux/videodev2.h sys/types.h)
PLAYERDRIVER_ADD_DRIVER (sphereptz build_sphereptz SOURCES sphereptz.cc)
