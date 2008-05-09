ADD_LIBRARY (multidriver SHARED multidriver.cc)
TARGET_LINK_LIBRARIES (multidriver playercore)

SET (exampleCMakeLists_in ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.example.in)
SET (exampleCMakeLists ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.example)
SET (exampleDestDir share/${PROJECT_NAME_LOWER}/examples/plugins/multidriver)
CONFIGURE_FILE (${exampleCMakeLists_in} ${exampleCMakeLists} @ONLY)
INSTALL (FILES ${exampleCMakeLists} DESTINATION ${exampleDestDir} RENAME CMakeLists.txt)
INSTALL (FILES multidriver.cc multi.cfg README DESTINATION ${exampleDestDir})
