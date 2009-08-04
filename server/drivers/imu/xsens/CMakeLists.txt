PLAYERDRIVER_OPTION (XSensMT build_XSensMT ON)
PLAYERDRIVER_REQUIRE_ENVVAR (XSensMT build_XSensMT XSENSMT_SOURCE_DIR xsensmtSrcDir)
SET (xsensmtSrcs XSensMT.cc
                 ${xsensmtSrcDir}/xsens_list.cpp
                 ${xsensmtSrcDir}/xsens_time.cpp
                 ${xsensmtSrcDir}/xsens_std.cpp
                 ${xsensmtSrcDir}/cmt1.cpp
                 ${xsensmtSrcDir}/cmt2.cpp
                 ${xsensmtSrcDir}/cmt3.cpp
                 ${xsensmtSrcDir}/cmtscan.cpp
                 ${xsensmtSrcDir}/cmtmessage.cpp
                 ${xsensmtSrcDir}/cmtpacket.cpp)
PLAYERDRIVER_ADD_DRIVER (XSensMT build_XSensMT SOURCES ${xsensmtSrcs})
