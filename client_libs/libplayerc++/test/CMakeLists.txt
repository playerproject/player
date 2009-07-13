IF (BUILD_PLAYERCC)
  IF (BUILD_PLAYERCC_BOOST)
    OPTION (BUILD_CC_TESTS "Build the C++ client library tests" ON)

    IF (BUILD_CC_TESTS)
        # Possibly use our own copy of XDR, and need gettimeofday replacement
        IF (NOT HAVE_XDR OR NOT HAVE_GETTIMEOFDAY OR NOT HAVE_USLEEP)
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
        ENDIF (NOT HAVE_XDR OR NOT HAVE_GETTIMEOFDAY OR NOT HAVE_USLEEP)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs ${PROJECT_BINARY_DIR}/client_libs)
        SET (testsSrcs  test.cc
                        test.h
                        test_camera.cc
                        test_rfid.cc
                        test_wsn.cc
                        test_gripper.cc
                        test_laser.cc
                        test_position2d.cc
                        test_ptz.cc
                        test_sonar.cc
                        test_power.cc
                        test_dio.cc
                        test_bumper.cc
                        test_position2d_holdsub.cc
                        test_gripper_holdsub.cc
                        test_actarray.cc
                        test_aio.cc
                        test_speech.cc
                        test_ranger.cc)

        IF (NOT HAVE_USLEEP)
            SET (playerreplaceLib playerreplace)
        ENDIF (NOT HAVE_USLEEP)
        IF (NOT HAVE_ROUND)
            SET (playerreplaceLib playerreplace)
        ENDIF (NOT HAVE_ROUND)

        LINK_DIRECTORIES (${PLAYERCC_EXTRA_LINK_DIRS})
        ADD_EXECUTABLE (playerc++_test ${testsSrcs})
        TARGET_LINK_LIBRARIES (playerc++_test playerc playerc++ playerinterface
                               playercommon playerwkb ${playerreplaceLib}
                               ${PLAYERCC_EXTRA_LINK_LIBRARIES})
    ENDIF (BUILD_CC_TESTS)
  ENDIF (BUILD_PLAYERCC_BOOST)
ENDIF (BUILD_PLAYERCC)
