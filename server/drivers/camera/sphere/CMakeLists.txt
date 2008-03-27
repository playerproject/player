PLAYERDRIVER_OPTION (sphere build_sphere ON)
PLAYERDRIVER_REQUIRE_HEADER (sphere build_sphere linux/videodev.h)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (sphere build_sphere sphere_mixed.cc setpwc_api.c)