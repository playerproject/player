PLAYERDRIVER_OPTION (cmucam2 build_cmucam2 ON)
PLAYERDRIVER_ADD_DRIVER (cmucam2 build_cmucam2
    CFLAGS "-I${PROJECT_SOURCE_DIR}/libplayercore"
    SOURCES cmucam2.cc camera.c)