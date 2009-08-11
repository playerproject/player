INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)

IF (PLAYER_OS_WIN)
    INSTALL (FILES rpc/xdr.h rpc/types.h DESTINATION ${PLAYER_INCLUDE_INSTALL_DIR}/rpc COMPONENT headers)
    INSTALL (FILES replace.h DESTINATION ${PLAYER_INCLUDE_INSTALL_DIR} COMPONENT headers)
ENDIF (PLAYER_OS_WIN)

IF (NOT HAVE_XDR)
    SET (replaceSrcs    ${replaceSrcs}
                        xdr.c
                        xdr_array.c
                        xdr_float.c
                        xdr_intXX_t.c
                        xdr_mem.c
                        xdr_sizeof.c)
    SET (ws2_32lib Ws2_32)
ENDIF (NOT HAVE_XDR)

IF (NOT HAVE_GETOPT)
    SET (replaceSrcs ${replaceSrcs} getopt.c)
ENDIF (NOT HAVE_GETOPT)

IF (NOT HAVE_DIRNAME)
    SET (replaceSrcs ${replaceSrcs} dirname.c)
ENDIF (NOT HAVE_DIRNAME)

IF (NOT HAVE_CLOCK_GETTIME)
    SET (replaceSrcs ${replaceSrcs} clock_gettime.c)
ENDIF (NOT HAVE_CLOCK_GETTIME)

IF (NOT HAVE_GETTIMEOFDAY)
    SET (replaceSrcs ${replaceSrcs} gettimeofday.c)
ENDIF (NOT HAVE_GETTIMEOFDAY)

IF (NOT HAVE_CFMAKERAW)
    SET (replaceSrcs ${replaceSrcs} cfmakeraw.c)
ENDIF (NOT HAVE_CFMAKERAW)

IF (NOT HAVE_ROUND)
    SET (replaceSrcs ${replaceSrcs} round.c)
ELSEIF (NOT HAVE_ROUND)
    IF (NOT HAVE_RINT)
        SET (replaceSrcs ${replaceSrcs} round.c)
    ENDIF (NOT HAVE_RINT)
ENDIF (NOT HAVE_ROUND)

IF (NOT HAVE_USLEEP)
    SET (replaceSrcs ${replaceSrcs} usleep.c)
ENDIF (NOT HAVE_USLEEP)

IF (NOT HAVE_NANOSLEEP)
    SET (replaceSrcs ${replaceSrcs} nanosleep.c)
ENDIF (NOT HAVE_NANOSLEEP)

IF (NOT HAVE_POLL)
    SET (replaceSrcs ${replaceSrcs} poll.c)
    SET (ws2_32lib Ws2_32)
ENDIF (NOT HAVE_POLL)

IF (NOT HAVE_COMPRESSBOUND)
    SET (replaceSrcs ${replaceSrcs} compressBound.c)
ENDIF (NOT HAVE_COMPRESSBOUND)

IF (replaceSrcs)
    PLAYER_ADD_LIBRARY (playerreplace ${replaceSrcs})
    TARGET_LINK_LIBRARIES (playerreplace playercommon)
    IF (NOT HAVE_XDR OR NOT HAVE_POLL)
        TARGET_LINK_LIBRARIES (playerreplace Ws2_32)
    ENDIF (NOT HAVE_XDR OR NOT HAVE_POLL)
    PLAYER_MAKE_PKGCONFIG ("playerreplace" "Player library of replacements for missing platform functionality - part of the Player Project" "playercommon ${ws2_32Lib}" "" "" "")
ENDIF (replaceSrcs)

