PLAYERDRIVER_OPTION (sphere build_sphere ON)
PLAYERDRIVER_REJECT_OS (sphere build_sphere PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (sphere build_sphere linux/videodev.h sys/types.h)
PLAYERDRIVER_ADD_DRIVER (sphere build_sphere
    CFLAGS "-I${PROJECT_SOURCE_DIR}/server/drivers/camera/v4l"
    SOURCES sphere_mixed.cc setpwc_api.c)
