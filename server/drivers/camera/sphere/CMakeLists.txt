PLAYERDRIVER_OPTION (sphere build_sphere ON)
PLAYERDRIVER_REQUIRE_HEADER (sphere build_sphere linux/videodev.h)
PLAYERDRIVER_ADD_DRIVER (sphere build_sphere
    "" "" "" "-I${PROJECT_SOURCE_DIR}/server/drivers/camera/v4l"
    sphere_mixed.cc setpwc_api.c)