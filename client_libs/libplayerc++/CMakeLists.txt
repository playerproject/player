ADD_SUBDIRECTORY (test)

OPTION (BUILD_PLAYERCC "Build the C++ client library" ON)
OPTION (BUILD_PLAYERCC_BOOST "Include Boost support in the C++ client library" ON)
IF (BUILD_PLAYERCC)
    # Look for Boost libraries
    IF (BUILD_PLAYERCC_BOOST)
        IF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
            # There is a new, much better, FindBoost.cmake in 2.6 (see below)
            FIND_PACKAGE (Boost)
            IF (Boost_FOUND)
                # For 2.4, assume that if boost is found then both signals and threads are present
                SET (HAVE_BOOST_THREAD TRUE INTERNAL)
                IF (HAVE_BOOST_THREAD)
                    OPTION (USE_BOOST_THREAD "Use the Boost threading library" ON)
                    MARK_AS_ADVANCED (USE_BOOST_THREAD)
                    IF (USE_BOOST_THREAD)
                        MESSAGE (STATUS
                             "PlayerC++ client library will be built with Boost::Thread support.")
                        PLAYERCC_ADD_LINK_LIB (boost_thread)
                        INCLUDE_DIRECTORIES (${Boost_INCLUDE_DIRS})
                        LINK_DIRECTORIES (${Boost_LIBRARY_DIRS})
                        SET (boostThreadLib -lboost_thread)
                    ELSE (USE_BOOST_THREAD)
                        MESSAGE (STATUS
                             "PlayerC++ client library Boost::Thread support disabled by user.")
                    ENDIF (USE_BOOST_THREAD)
                ELSE (HAVE_BOOST_THREAD)
                    MESSAGE (STATUS
                             "PlayerC++ client library Boost::Thread support disabled -
                             Boost::Thread library not found.")
                ENDIF (HAVE_BOOST_THREAD)

                SET (HAVE_BOOST_SIGNALS TRUE INTERNAL)
                IF (HAVE_BOOST_SIGNALS)
                    OPTION (USE_BOOST_SIGNALS "Use the Boost signalling library" ON)
                    MARK_AS_ADVANCED (USE_BOOST_SIGNALS)
                    IF (USE_BOOST_SIGNALS)
                        MESSAGE (STATUS
                             "PlayerC++ client library will be built with Boost::Signals support.")
                        PLAYERCC_ADD_LINK_LIB (boost_signals)
                        INCLUDE_DIRECTORIES (${Boost_INCLUDE_DIRS})
                        LINK_DIRECTORIES (${Boost_LIBRARY_DIRS})
                        SET (boostSignalsLib -lboost_signals)
                    ELSE (USE_BOOST_SIGNALS)
                        MESSAGE (STATUS
                                 "PlayerC++ client library Boost::Signals support disabled by
                                 user.")
                    ENDIF (USE_BOOST_SIGNALS)
                ELSE (HAVE_BOOST_SIGNALS)
                    MESSAGE (STATUS
                             "PlayerC++ client library Boost::Signals support disabled -
                             Boost::Signals library not found.")
                ENDIF (HAVE_BOOST_SIGNALS)
            ELSE (Boost_FOUND)
                MESSAGE (STATUS
                         "Boost libraries were not found. Boost::Signals and Boost::Thread support
                         will not be included in PlayerC++.")
            ENDIF (Boost_FOUND)
        ELSE (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
            OPTION (Boost_USE_STATIC_LIBS "Use the static versions of the Boost libraries" OFF)
            OPTION (USE_BOOST_THREAD "Use the Boost threading library" ON)
            MARK_AS_ADVANCED (USE_BOOST_THREAD)
            OPTION (USE_BOOST_SIGNALS "Use the Boost signalling library" ON)
            MARK_AS_ADVANCED (USE_BOOST_SIGNALS)

            IF (USE_BOOST_THREAD)
                SET (BOOST_COMPONENTS thread)
            ENDIF (USE_BOOST_THREAD)
            IF (USE_BOOST_SIGNALS)
                SET (BOOST_COMPONENTS ${BOOST_COMPONENTS} signals)
            ENDIF (USE_BOOST_SIGNALS)
            FIND_PACKAGE (Boost COMPONENTS ${BOOST_COMPONENTS})
            IF (Boost_FOUND)
                INCLUDE_DIRECTORIES (${Boost_INCLUDE_DIR})
                LINK_DIRECTORIES (${Boost_LIBRARY_DIRS})

                IF (Boost_thread_FOUND)
                    SET (boostThreadLib -lboost_thread)
                    PLAYERCC_ADD_LINK_LIB (boost_thread)
                    MESSAGE (STATUS
                            "PlayerC++ client library will be built with Boost::Thread support.")
                ELSE (Boost_thread_FOUND)
                    MESSAGE (STATUS
                            "Boost::Thread library not found, support will not be included.")
                ENDIF (Boost_thread_FOUND)

                IF (Boost_signals_FOUND)
                    SET (boostSignalsLib -lboost_signals)
                    PLAYERCC_ADD_LINK_LIB (boost_signals)
                    MESSAGE (STATUS
                            "PlayerC++ client library will be built with Boost::Signals support.")
                ELSE (Boost_signals_FOUND)
                    MESSAGE (STATUS
                            "Boost::Signals library not found, support will not be included.")
                ENDIF (Boost_signals_FOUND)
            ELSE (Boost_FOUND)
                MESSAGE (STATUS
                         "Boost libraries were not found. Boost::Signals and Boost::Thread support
                         will not be included in PlayerC++.")
            ENDIF (Boost_FOUND)
        ENDIF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
    ELSE (BUILD_PLAYERCC_BOOST)
        MESSAGE (STATUS
                 "Boost support disabled. Boost::Signals and Boost::Thread support will not be
                 included in PlayerC++.")
    ENDIF (BUILD_PLAYERCC_BOOST)

    # Auto-generate playerc++config.h.
    SET (playercppconfig_h_in "${CMAKE_CURRENT_SOURCE_DIR}/playerc++config.h.in")
    SET (playercppconfig_h "${CMAKE_CURRENT_BINARY_DIR}/playerc++config.h")
    CONFIGURE_FILE (${playercppconfig_h_in} ${playercppconfig_h})

    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs
                         ${PROJECT_BINARY_DIR}/client_libs)

    SET (playerccSrcs   playerc++.h
                        debug.h
                        playerclient.h
                        playererror.h
                        playerc++.cc
                        playerclient.cc
                        playererror.cc
                        clientproxy.cc
                        actarrayproxy.cc
                        aioproxy.cc
                        audioproxy.cc
                        blackboardproxy.cc
                        blobfinderproxy.cc
                        bumperproxy.cc
                        cameraproxy.cc
                        dioproxy.cc
                        fiducialproxy.cc
                        gpsproxy.cc
                        graphics2dproxy.cc
                        graphics3dproxy.cc
                        gripperproxy.cc
                        healthproxy.cc
                        imuproxy.cc
                        irproxy.cc
                        laserproxy.cc
                        limbproxy.cc
                        localizeproxy.cc
                        logproxy.cc
                        mapproxy.cc
                        opaqueproxy.cc
                        plannerproxy.cc
                        pointcloud3d.cc
                        position1dproxy.cc
                        position2dproxy.cc
                        position3dproxy.cc
                        powerproxy.cc
                        ptzproxy.cc
                        rangerproxy.cc
                        rfidproxy.cc
                        simulationproxy.cc
                        sonarproxy.cc
                        speechproxy.cc
                        speechrecognitionproxy.cc
                        vectormapproxy.cc
                        wifiproxy.cc
                        wsnproxy.cc)

    PLAYER_ADD_LIBRARY (playerc++ ${playerccSrcs} ${playercppconfig_h})
    TARGET_LINK_LIBRARIES (playerc++ playerxdr playerutils playerc ${PLAYERCC_EXTRA_LINK_LIBRARIES})
    PLAYER_MAKE_PKGCONFIG ("playerc++"
                           "C++ wrapper for libplayerc - part of the Player Project"
                           "playerxdr playerc" "" "" "-lm ${boostThreadLib} ${boostSignalsLib}")

    PLAYER_INSTALL_HEADERS (playerc++ ${playercppconfig_h}
                                      clientproxy.h
                                      playerc++.h
                                      playerclient.h
                                      playererror.h
                                      utility.h)
ENDIF (BUILD_PLAYERCC)
