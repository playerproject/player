IF (BUILD_PLAYERCC)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${PROJECT_BINARY_DIR}/client_libs)

    SET (exampleSrcs args.h camera.cc example0.cc example4.cc grip.cc clientgraphics.cc
                     clientgraphics3d.cc laserobstacleavoid.cc ptz.cc randomwalk.cc
                     sonarobstacleavoid.cc speech.cc wallfollow.cc)
    SET (exampleBoostSrcs example1.cc example3.cc goto.cc speech_cpp_client.cc)
    SET (exampleBoostSigSrcs example2.cc)

    IF (NOT HAVE_GETOPT)
        SET (playerreplaceLib playerreplace)
    ENDIF (NOT HAVE_GETOPT)

    ADD_DEFINITIONS (${PLAYERCC_DEFINITIONS})
    INCLUDE_DIRECTORIES (${PLAYERCC_EXTRA_INCLUDE_DIRS})
    LINK_DIRECTORIES (${PLAYERCC_EXTRA_LINK_DIRS})
    MACRO (PLAYERCPP_ADD_EXAMPLE _name)
        ADD_EXECUTABLE (${_name} ${ARGN})
        TARGET_LINK_LIBRARIES (${_name} playerc++ playerc playerinterface playercommon
                               ${playerreplaceLib} ${PLAYERC_EXTRA_LINK_LIBRARIES}
                               ${PLAYERCC_EXTRA_LINK_LIBRARIES})
        IF (PLAYER_OS_SOLARIS)
            TARGET_LINK_LIBRARIES (${_name} rt)
        ENDIF (PLAYER_OS_SOLARIS)
    ENDMACRO (PLAYERCPP_ADD_EXAMPLE _name)

    PLAYERCPP_ADD_EXAMPLE (camera camera.cc)
    PLAYERCPP_ADD_EXAMPLE (example0 example0.cc)
    PLAYERCPP_ADD_EXAMPLE (example4 example4.cc)
    PLAYERCPP_ADD_EXAMPLE (grip grip.cc)
    PLAYERCPP_ADD_EXAMPLE (clientgraphics clientgraphics.cc)
    PLAYERCPP_ADD_EXAMPLE (clientgraphics3d clientgraphics3d.cc)
    PLAYERCPP_ADD_EXAMPLE (laserobstacleavoid laserobstacleavoid.cc)
    PLAYERCPP_ADD_EXAMPLE (ptz ptz.cc)
    PLAYERCPP_ADD_EXAMPLE (randomwalk randomwalk.cc)
    PLAYERCPP_ADD_EXAMPLE (sonarobstacleavoid sonarobstacleavoid.cc)
    PLAYERCPP_ADD_EXAMPLE (speech speech.cc)
    PLAYERCPP_ADD_EXAMPLE (wallfollow wallfollow.cc)

    INSTALL (FILES ${exampleSrcs} DESTINATION share/${PROJECT_NAME_LOWER}/examples/libplayerc++ COMPONENT examples)

    IF (BUILD_PLAYERCC_BOOST)
      IF (HAVE_BOOST_THREAD AND HAVE_BOOST_SIGNALS)
          PLAYERCPP_ADD_EXAMPLE (example1 example1.cc)
          PLAYERCPP_ADD_EXAMPLE (example3 example3.cc)
          PLAYERCPP_ADD_EXAMPLE (goto goto.cc)
          PLAYERCPP_ADD_EXAMPLE (speech_cpp_client speech_cpp_client.cc)
          INSTALL (FILES ${exampleBoostSrcs} DESTINATION share/${PROJECT_NAME_LOWER}/examples/libplayerc++ COMPONENT examples)
      ENDIF (HAVE_BOOST_THREAD AND HAVE_BOOST_SIGNALS)

      IF (HAVE_BOOST_SIGNALS)
          PLAYERCPP_ADD_EXAMPLE (example2 example2.cc)
          INSTALL (FILES ${exampleBoostSigSrcs} DESTINATION share/${PROJECT_NAME_LOWER}/examples/libplayerc++ COMPONENT examples)
      ENDIF (HAVE_BOOST_SIGNALS)
    ENDIF (BUILD_PLAYERCC_BOOST)

    # Install the example CMakeLists.txt
    SET (exampleCMakeLists_in ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.example.in)
    SET (exampleCMakeLists ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.example)
    CONFIGURE_FILE (${exampleCMakeLists_in} ${exampleCMakeLists} @ONLY)
    INSTALL (FILES ${exampleCMakeLists} DESTINATION share/${PROJECT_NAME_LOWER}/examples/libplayerc++ RENAME CMakeLists.txt COMPONENT examples)
    INSTALL (FILES README DESTINATION share/${PROJECT_NAME_LOWER}/examples/libplayerc++ COMPONENT examples)
ELSE (BUILD_PLAYERCC)
    MESSAGE (STATUS "playerc++ examples will not be built - playerc++ client library is disabled")
ENDIF (BUILD_PLAYERCC)
