CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/UsePlayerC.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/UsePlayerC.cmake @ONLY)
SET (playerModules ${CMAKE_CURRENT_BINARY_DIR}/UsePlayerC.cmake
                   UsePlayerPlugin.cmake
                   PlayerUtils.cmake)
INSTALL (FILES ${playerModules} DESTINATION share/cmake/Modules/)

IF (BUILD_PLAYERCC)
    INSTALL (FILES UsePlayerC++.cmake DESTINATION share/cmake/Modules/)
ENDIF (BUILD_PLAYERCC)
