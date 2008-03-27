SET (playerjpegSrcs playerjpeg.c)

PLAYER_ADD_LIBRARY (playerjpeg ${playerjpegSrcs})
TARGET_LINK_LIBRARIES (jpeg)
PLAYER_ADD_LINK_LIB (jpeg)
PLAYER_INSTALL_HEADERS (playerjpeg playerjpeg.h)