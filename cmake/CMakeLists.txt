CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/UsePlayerC.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/UsePlayerC.cmake @ONLY)
# UsePlayerPlugin.cmake.in is configured in playercore
SET (playerModules ${CMAKE_CURRENT_BINARY_DIR}/UsePlayerC.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/UsePlayerPlugin.cmake
                   PlayerUtils.cmake)
INSTALL (FILES ${playerModules} DESTINATION share/cmake/Modules/ COMPONENT cmakestuff)

IF (BUILD_PLAYERCC)
    INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/UsePlayerC++.cmake DESTINATION share/cmake/Modules/ COMPONENT cmakestuff)
ENDIF (BUILD_PLAYERCC)
