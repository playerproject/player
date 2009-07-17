IF (HAVE_STL)
    PLAYERDRIVER_OPTION (laserptzcloud build_laserptzcloud ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (laserptzcloud build_laserptzcloud OFF "STL not found")
ENDIF (HAVE_STL)

PLAYERDRIVER_ADD_DRIVER (laserptzcloud build_laserptzcloud SOURCES laserptzcloud.cc)
