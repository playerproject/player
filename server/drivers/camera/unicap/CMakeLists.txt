PLAYERDRIVER_OPTION (unicapimage build_unicapimage ON)

SET (UNICAP_DIR "" CACHE STRING "Directory containing the UniCap headers and libraries")
MARK_AS_ADVANCED (UNICAP_DIR)
IF ("${UNICAP_DIR}" STREQUAL "")
    SET (unicapReqHeader "unicap.h")
    SET (unicapExtraFlags "")
    SET (unicapExtraLibs "-lunicap -lrt")
ELSE ("${UNICAP_DIR}" STREQUAL "")
    SET (unicapReqHeader "${UNICAP_DIR}/include/unicap/unicap.h")
    SET (unicapExtraFlags "-I${UNICAP_DIR}/include/unicap")
    SET (unicapExtraLibs "-L${UNICAP_DIR}/lib -lcanlib -lrt")
ENDIF ("${UNICAP_DIR}" STREQUAL "")

PLAYERDRIVER_REQUIRE_HEADER (unicapimage build_unicapimage ${unicapReqHeader})

PLAYERDRIVER_ADD_DRIVER (unicapimage build_unicapimage
                         LINKFLAGS -L{UNICAP_DIR}/lib -lunicap -lrt
                         CFLAGS -I${UNICAP_DIR}/include -I${UNICAP_DIR}/include/unicap
                         SOURCES unicapImage.cc)
