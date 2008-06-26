PLAYERDRIVER_OPTION (cmvision build_cmvision ON)
PLAYERDRIVER_ADD_DRIVER (cmvision build_cmvision 
  CFLAGS "-DUSE_METEOR"
  SOURCES cmvision.cc conversions.c P2CMV.cc )
