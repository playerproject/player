ADD_SUBDIRECTORY (test)
ADD_SUBDIRECTORY (bindings)

# Possibly use our own copy of XDR, and need gettimeofday replacement
IF (NOT HAVE_XDR OR NOT HAVE_GETTIMEOFDAY)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
ENDIF (NOT HAVE_XDR OR NOT HAVE_GETTIMEOFDAY)

IF (HAVE_STL)
    OPTION (BUILD_PLAYERCC "Build the C++ client library" ON)
ELSE (HAVE_STL)
    MESSAGE (STATUS "PlayerC++ client library will not be built - STL is not available.")
    SET (BUILD_PLAYERCC OFF CACHE BOOL "Build the C++ client library" FORCE)
ENDIF (HAVE_STL)

IF (PLAYER_OS_OSX AND CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
    # Boost is off by default on OS X under CMake 2.4
    OPTION (BUILD_PLAYERCC_BOOST "Include Boost support in the C++ client library" OFF)
ELSE (PLAYER_OS_OSX AND CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)
    OPTION (BUILD_PLAYERCC_BOOST "Include Boost support in the C++ client library" ON)
ENDIF (PLAYER_OS_OSX AND CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 4)

IF (BUILD_PLAYERCC)
    # Look for Boost libraries
    IF (BUILD_PLAYERCC_BOOST)
        # There is a new, much better, FindBoost.cmake in 2.6
        IF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 6)
            OPTION (Boost_USE_STATIC_LIBS "Use the static versions of the Boost libraries" OFF)
            MARK_AS_ADVANCED (Boost_USE_STATIC_LIBS)
            OPTION (USE_BOOST_THREAD "Use the Boost threading library" ON)
            MARK_AS_ADVANCED (USE_BOOST_THREAD)
            OPTION (USE_BOOST_SIGNALS "Use the Boost signalling library" ON)
            MARK_AS_ADVANCED (USE_BOOST_SIGNALS)

            IF (USE_BOOST_THREAD)
                SET (BOOST_COMPONENTS thread)
            ELSE (USE_BOOST_THREAD)
                SET (HAVE_BOOST_THREAD FALSE CACHE INTERNAL "Is the Boost::Thread library present")
            ENDIF (USE_BOOST_THREAD)
            IF (USE_BOOST_SIGNALS)
                SET (BOOST_COMPONENTS ${BOOST_COMPONENTS} signals)
            ELSE (USE_BOOST_SIGNALS)
                SET (HAVE_BOOST_SIGNALS FALSE CACHE INTERNAL "Is the Boost::Signals library present")
            ENDIF (USE_BOOST_SIGNALS)
            FIND_PACKAGE (Boost COMPONENTS ${BOOST_COMPONENTS})
            IF (Boost_FOUND)
                PLAYERCC_ADD_INCLUDE_DIR (${Boost_INCLUDE_DIR})
                PLAYERCC_ADD_LINK_DIR (${Boost_LIBRARY_DIRS})

                IF (Boost_THREAD_FOUND)
                    GET_FILENAME_COMPONENT (boostThreadLib ${Boost_THREAD_LIBRARY} NAME_WE CACHE)
                    # Chop off the lib at the front, too, if present
                    STRING (REGEX REPLACE "^lib" "" boostThreadLib ${boostThreadLib})
                    PLAYERCC_ADD_LINK_LIB (${boostThreadLib})
                    SET (boostIncludeDir ${Boost_INCLUDE_DIR})
                    SET (HAVE_BOOST_THREAD TRUE CACHE INTERNAL "Is the Boost::Thread library present")
                    MESSAGE (STATUS
                            "PlayerC++ client library will be built with Boost::Thread support.")
                ELSE (Boost_THREAD_FOUND)
                    MESSAGE (STATUS
                            "Boost::Thread library not found, support will not be included.")
                ENDIF (Boost_THREAD_FOUND)

                IF (Boost_SIGNALS_FOUND)
                    GET_FILENAME_COMPONENT (boostSignalsLib ${Boost_SIGNALS_LIBRARY} NAME_WE CACHE)
                    STRING (REGEX REPLACE "^lib" "" boostSignalsLib ${boostSignalsLib})
                    PLAYERCC_ADD_LINK_LIB (${boostSignalsLib})
                    SET (boostIncludeDir ${Boost_INCLUDE_DIR})
                    SET (HAVE_BOOST_SIGNALS TRUE CACHE INTERNAL "Is the Boost::Signals library present")
                    MESSAGE (STATUS
                            "PlayerC++ client library will be built with Boost::Signals support.")
                ELSE (Boost_SIGNALS_FOUND)
                    MESSAGE (STATUS
                            "Boost::Signals library not found, support will not be included.")
                ENDIF (Boost_SIGNALS_FOUND)
            ELSE (Boost_FOUND)
                MESSAGE (STATUS
                         "Boost libraries were not found. Boost::Signals and Boost::Thread support
                         will not be included in PlayerC++.")
            ENDIF (Boost_FOUND)
        ELSE (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 6)
            FIND_PACKAGE (Boost)
            IF (Boost_FOUND)
                # For 2.4, assume that if boost is found then both signals and threads are present
                SET (HAVE_BOOST_THREAD TRUE CACHE INTERNAL "Is the Boost::Thread library present")
                IF (PLAYER_OS_OSX)
                    OPTION (Boost_USE_MULTITHREAD "Use the multithreaded versions of the Boost libraries" ON)
                ELSE (PLAYER_OS_OSX)
                    OPTION (Boost_USE_MULTITHREAD "Use the multithreaded versions of the Boost libraries" OFF)
                ENDIF (PLAYER_OS_OSX)
                MARK_AS_ADVANCED (Boost_USE_MULTITHREAD)
                IF (Boost_USE_MULTITHREAD)
                    SET (BOOST_LIB_SUFFIX "-mt" CACHE STRING "Boost library name suffix")
                ELSE (Boost_USE_MULTITHREAD)
                    SET (BOOST_LIB_SUFFIX "" CACHE STRING "Boost library name suffix")
                ENDIF (Boost_USE_MULTITHREAD)
                MARK_AS_ADVANCED (BOOST_LIB_SUFFIX)

                IF (HAVE_BOOST_THREAD)
                    OPTION (USE_BOOST_THREAD "Use the Boost threading library" ON)
                    MARK_AS_ADVANCED (USE_BOOST_THREAD)
                    IF (USE_BOOST_THREAD)
                        MESSAGE (STATUS
                             "PlayerC++ client library will be built with Boost::Thread support.")
                        SET (boostThreadLib boost_thread${BOOST_LIB_SUFFIX})
                        PLAYERCC_ADD_LINK_LIB (${boostThreadLib})
                        PLAYERCC_ADD_INCLUDE_DIR (${Boost_INCLUDE_DIR})
                        PLAYERCC_ADD_LINK_DIR (${Boost_LIBRARY_DIRS})
                        SET (boostIncludeDir ${Boost_INCLUDE_DIRS})
                    ELSE (USE_BOOST_THREAD)
                        SET (HAVE_BOOST_THREAD FALSE)
                        MESSAGE (STATUS
                             "PlayerC++ client library Boost::Thread support disabled by user.")
                    ENDIF (USE_BOOST_THREAD)
                ELSE (HAVE_BOOST_THREAD)
                    MESSAGE (STATUS
                             "PlayerC++ client library Boost::Thread support disabled -
                             Boost::Thread library not found.")
                ENDIF (HAVE_BOOST_THREAD)

                SET (HAVE_BOOST_SIGNALS TRUE CACHE INTERNAL "Is the Boost::Signals library present")
                IF (HAVE_BOOST_SIGNALS)
                    OPTION (USE_BOOST_SIGNALS "Use the Boost signalling library" ON)
                    MARK_AS_ADVANCED (USE_BOOST_SIGNALS)
                    IF (USE_BOOST_SIGNALS)
                        MESSAGE (STATUS
                             "PlayerC++ client library will be built with Boost::Signals support.")
                        SET (boostSignalsLib boost_signals${BOOST_LIB_SUFFIX})
                        PLAYERCC_ADD_LINK_LIB (${boostSignalsLib})
                        PLAYERCC_ADD_INCLUDE_DIR (${Boost_INCLUDE_DIR})
                        PLAYERCC_ADD_LINK_DIR (${Boost_LIBRARY_DIRS})
                        SET (boostIncludeDir ${Boost_INCLUDE_DIRS})
                    ELSE (USE_BOOST_SIGNALS)
                        SET (HAVE_BOOST_SIGNALS FALSE)
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
        ENDIF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 6)
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
                        joystickproxy.cc
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

    IF (NOT HAVE_NANOSLEEP)
        SET (playerreplaceLib playerreplace)
    ENDIF (NOT HAVE_NANOSLEEP)

    IF (HAVE_M)
        PLAYERCC_ADD_LINK_LIB (m)
    ENDIF (HAVE_M)

    IF (PLAYER_OS_SOLARIS)
        PLAYERCC_ADD_LINK_LIB ("socket")
        PLAYERCC_ADD_LINK_LIB ("nsl")
    ENDIF (PLAYER_OS_SOLARIS)

    INCLUDE_DIRECTORIES (${PLAYERCC_EXTRA_INCLUDE_DIRS})
    LINK_DIRECTORIES (${PLAYERCC_EXTRA_LINK_DIRS})
    PLAYER_ADD_LIBRARY (playerc++ ${playerccSrcs} ${playercppconfig_h})
    TARGET_LINK_LIBRARIES (playerc++ playerwkb playercommon playerinterface playerc
                           ${playerreplaceLib} ${PLAYERCC_EXTRA_LINK_LIBRARIES})

    SET (pkgconfigCFlags)
    SET (pkgconfigLinkDirs)
    SET (pkgconfigLinkLibs)
    LIST_TO_STRING_WITH_PREFIX (pkgconfigCFlags "-I" ${PLAYERCC_EXTRA_INCLUDE_DIRS})
    LIST_TO_STRING_WITH_PREFIX (pkgconfigLinkDirs "-L" ${PLAYERCC_EXTRA_LINK_DIRS})
    LIST_TO_STRING_WITH_PREFIX (pkgconfigLinkLibs "-l" ${PLAYERCC_EXTRA_LINK_LIBRARIES})
    PLAYER_MAKE_PKGCONFIG ("playerc++"
                           "C++ wrapper for libplayerc - part of the Player Project"
                           "playerwkb playerinterface playerc ${playerreplaceLib}" ""
                           "${pkgconfigCFlags}" "${pkgconfigLinkDirs} ${pkgconfigLinkLibs}")
    CONFIGURE_FILE (${PLAYER_CMAKE_DIR}/UsePlayerC++.cmake.in ${CMAKE_BINARY_DIR}/cmake/UsePlayerC++.cmake @ONLY)

    PLAYER_INSTALL_HEADERS (playerc++ ${playercppconfig_h}
                                      clientproxy.h
                                      playerc++.h
                                      playerclient.h
                                      playererror.h
                                      utility.h)
ENDIF (BUILD_PLAYERCC)

