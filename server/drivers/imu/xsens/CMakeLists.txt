PLAYERDRIVER_OPTION (xsensmt build_xsensmt OFF "Disabled by default - set XSENSMT_SOURCE_DIR")
SET (XSENSMT_SOURCE_DIR "" CACHE FILEPATH "Directory containing the XSensMT manufacturer source files")
MARK_AS_ADVANCED (XSENSMT_SOURCE_DIR)
SET (xsensmtSrcs XSensMT.cc
                 ${XSENSMT_SOURCE_DIR}/xsens_time.cpp
                 ${XSENSMT_SOURCE_DIR}/xsens_std.cpp
                 ${XSENSMT_SOURCE_DIR}/cmt1.cpp
                 ${XSENSMT_SOURCE_DIR}/cmt2.cpp
                 ${XSENSMT_SOURCE_DIR}/cmt3.cpp
                 ${XSENSMT_SOURCE_DIR}/cmtscan.cpp
                 ${XSENSMT_SOURCE_DIR}/cmtmessage.cpp
                 ${XSENSMT_SOURCE_DIR}/cmtpacket.cpp)
PLAYERDRIVER_ADD_DRIVER (xsensmt build_xsensmt SOURCES ${xsensmtSrcs}
                         INCLUDEDIRS ${XSENSMT_SOURCE_DIR})

