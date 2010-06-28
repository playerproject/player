PLAYERDRIVER_OPTION (cmdsplitter build_cmdsplitter ON)
PLAYERDRIVER_ADD_DRIVER (cmdsplitter build_cmdsplitter SOURCES cmdsplitter.cc)

PLAYERDRIVER_OPTION (diocmd build_diocmd ON)
PLAYERDRIVER_ADD_DRIVER (diocmd build_diocmd SOURCES diocmd.cc)

PLAYERDRIVER_OPTION (dummy build_dummy ON)
PLAYERDRIVER_ADD_DRIVER (dummy build_dummy SOURCES dummy.cc)

PLAYERDRIVER_OPTION (gripcmd build_gripcmd ON)
PLAYERDRIVER_ADD_DRIVER (gripcmd build_gripcmd SOURCES gripcmd.cc)

PLAYERDRIVER_OPTION (inhibitor build_inhibitor ON)
PLAYERDRIVER_ADD_DRIVER (inhibitor build_inhibitor SOURCES inhibitor.cc)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (kartowriter build_kartowriter ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (kartowriter build_kartowriter OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_ADD_DRIVER (kartowriter build_kartowriter SOURCES kartowriter.cc)

PLAYERDRIVER_OPTION (writelog build_writelog ON)
PLAYERDRIVER_ADD_DRIVER (writelog build_writelog SOURCES writelog.cc encode.cc)

PLAYERDRIVER_OPTION (readlog build_readlog ON)
IF (HAVE_Z)
    SET (readlogLinkFlags -lz)
ENDIF (HAVE_Z)
PLAYERDRIVER_ADD_DRIVER (readlog build_readlog SOURCES encode.cc readlog_time.cc readlog.cc
                        LINKFLAGS ${readlogLinkFlags})

PLAYERDRIVER_OPTION (passthrough build_passthrough ON)
PLAYERDRIVER_ADD_DRIVER (passthrough build_passthrough SOURCES passthrough.cc)

PLAYERDRIVER_OPTION (postlog build_postlog ON)
PLAYERDRIVER_REQUIRE_PKG (postlog build_postlog libpqxx
    postlog_includeDirs postlog_libDirs postlog_linkLibs postlog_linkFlags postlog_cFlags)
IF (PLAYER_OS_SOLARIS)
    SET (postlogLinkFlag "-lpq")
ELSE (PLAYER_OS_SOLARIS)
    SET (postlogLinkFlag)
ENDIF (PLAYER_OS_SOLARIS)
PLAYERDRIVER_ADD_DRIVER (postlog build_postlog
    INCLUDEDIRS ${postlog_includeDirs} LIBDIRS ${postlog_libDirs}
    LINKLIBS ${postlog_linkLibs} LINKFLAGS ${postlog_linkFlags} ${postlogLinkFlag}
    CFLAGS ${postlog_cFlags} SOURCES postlog.cc)

PLAYERDRIVER_OPTION (relay build_relay ON)
PLAYERDRIVER_ADD_DRIVER (relay build_relay SOURCES relay.cc)

PLAYERDRIVER_OPTION (suppressor build_suppressor ON)
PLAYERDRIVER_ADD_DRIVER (suppressor build_suppressor SOURCES suppressor.cc)

PLAYERDRIVER_OPTION (velcmd build_velcmd ON)
PLAYERDRIVER_ADD_DRIVER (velcmd build_velcmd SOURCES velcmd.cc)

PLAYERDRIVER_OPTION (opaquecmd build_opaquecmd ON)
PLAYERDRIVER_ADD_DRIVER (opaquecmd build_opaquecmd SOURCES opaquecmd.cc)

PLAYERDRIVER_OPTION (speechcmd build_speechcmd ON)
PLAYERDRIVER_ADD_DRIVER (speechcmd build_speechcmd SOURCES speechcmd.cc)
