PLAYERDRIVER_OPTION (guile build_guile ON)
PLAYERDRIVER_REJECT_OS (guile build_guile PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_PKG (guile build_guile guile-1.8
    guile_includeDirs guile_libDirs guile_linkLibs guile_linkFlags guile_cFlags)
PLAYERDRIVER_ADD_DRIVER (guile build_guile
    INCLUDEDIRS ${guile_includeDirs} LIBDIRS ${guile_libDirs} LINKLIBS ${guile_linkLibs}
    LINKFLAGS ${guile_linkFlags} CFLAGS ${guile_cFlags} -I${CMAKE_BINARY_DIR}/server/drivers/shell/guile
    SOURCES guile.cc ${CMAKE_BINARY_DIR}/server/drivers/shell/guile/reflection.cc)
