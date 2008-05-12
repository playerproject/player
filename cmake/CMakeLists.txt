SET (playerModules UsePlayerC.cmake
                   UsePlayerPlugin.cmake
                   PlayerUtils.cmake)
INSTALL (FILES ${playerModules} DESTINATION share/cmake/Modules/)

IF (BUILD_PLAYERCC)
    INSTALL (FILES UsePlayerC++.cmake DESTINATION share/cmake/Modules/)
ENDIF (BUILD_PLAYERCC)
