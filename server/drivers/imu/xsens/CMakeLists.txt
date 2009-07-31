SET (xsensmtSrcDir $ENV{XSENSMT_SOURCE_DIR})
IF (xsensmtSrcDir)
    PLAYERDRIVER_OPTION (XSensMT build_XSensMT ON)
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
ELSE (xsensmtSrcDir)
    PLAYERDRIVER_OPTION (XSensMT build_XSensMT OFF
    "Set the XSENSMT_SOURCE_DIR environment variable to the location of the provided source files.")
ENDIF (xsensmtSrcDir)
PLAYERDRIVER_ADD_DRIVER (XSensMT build_XSensMT SOURCES ${xsensmtSrcs})
