ADD_SUBDIRECTORY (test)

OPTION (BUILD_PLAYERCC "Build the C++ client library" ON)
IF (BUILD_PLAYERCC)
    SET (PLAYERCC_DEFINITIONS "" CACHE INTERNAL "" FORCE)   # Clear this out to prevent unnecessary duplication and rebuilding
    # Look for Boost libraries
    SET (boostLinkLibs)
    FIND_PACKAGE (Boost)
    IF (Boost_FOUND)
        # TODO: there may be a new, much better, FindBoost.cmake coming in 2.6
        # For now, assume that if boost is found then both signals and threads are present
        SET (HAVE_BOOST_THREAD TRUE INTERNAL)
        IF (HAVE_BOOST_THREAD)
            OPTION (USE_BOOST_THREADS "Use the Boost threading library" ON)
            MARK_AS_ADVANCED (USE_BOOST_THREADS)
            IF (USE_BOOST_THREADS)
                MESSAGE (STATUS "PlayerC++ client library will be built with Boost::Thread support.")
                PLAYERCC_ADD_LINK_LIB (boost_thread)
                INCLUDE_DIRECTORIES (${Boost_INCLUDE_DIRS})
                LINK_DIRECTORIES (${Boost_LIBRARY_DIRS})
                APPEND_TO_CACHED_STRING (PLAYERCC_DEFINITIONS "libplayerc++ defines" -DHAVE_BOOST_THREAD -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT)
                SET (boostLinkLibs "${boostLinkLibs} -lboost_thread")
            ELSE (USE_BOOST_THREADS)
                MESSAGE (STATUS "PlayerC++ client library Boost::Thread support disabled by user.")
            ENDIF (USE_BOOST_THREADS)
        ELSE (HAVE_BOOST_THREAD)
            MESSAGE (STATUS "PlayerC++ client library Boost::Thread support disabled - Boost::Thread library not found.")
        ENDIF (HAVE_BOOST_THREAD)

        SET (HAVE_BOOST_SIGNALS TRUE INTERNAL)
        IF (HAVE_BOOST_SIGNALS)
            OPTION (USE_BOOST_SIGNALS "Use the Boost signalling library" ON)
            MARK_AS_ADVANCED (USE_BOOST_SIGNALS)
            IF (USE_BOOST_SIGNALS)
                MESSAGE (STATUS "PlayerC++ client library will be built with Boost::Signals support.")
                PLAYERCC_ADD_LINK_LIB (boost_signals)
                INCLUDE_DIRECTORIES (${Boost_INCLUDE_DIRS})
                LINK_DIRECTORIES (${Boost_LIBRARY_DIRS})
                APPEND_TO_CACHED_STRING (PLAYERCC_DEFINITIONS "libplayerc++ defines" -DHAVE_BOOST_SIGNALS)
                SET (boostLinkLibs "${boostLinkLibs} -lboost_signals")
            ELSE (USE_BOOST_SIGNALS)
                MESSAGE (STATUS "PlayerC++ client library Boost::Signals support disabled by user.")
            ENDIF (USE_BOOST_SIGNALS)
        ELSE (HAVE_BOOST_SIGNALS)
            MESSAGE (STATUS "PlayerC++ client library Boost::Signals support disabled - Boost::Signals library not found.")
        ENDIF (HAVE_BOOST_SIGNALS)
    ELSE (Boost_FOUND)
        MESSAGE (STATUS "Boost libraries were not found. Boost::Signals and Boost::Thread support will not be included in PlayerC++.")
    ENDIF (Boost_FOUND)

    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

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

    ADD_DEFINITIONS (${PLAYERCC_DEFINITIONS})
    PLAYER_ADD_LIBRARY (playerc++ ${playerccSrcs})
    TARGET_LINK_LIBRARIES (playerc++ playerxdr playerutils playerc ${PLAYERCC_EXTRA_LINK_LIBRARIES})
    PLAYER_MAKE_PKGCONFIG ("playerc++" "C++ wrapper for libplayerc - part of the Player Project" "playerxdr playerc" "" "" "-lm ${boostLinkLibs}")

    PLAYER_INSTALL_HEADERS (playerc++ clientproxy.h playerc++.h playerclient.h playererror.h utility.h)
ENDIF (BUILD_PLAYERCC)