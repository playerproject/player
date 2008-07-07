PLAYERDRIVER_OPTION (bumper2laser build_bumper2laser ON)
PLAYERDRIVER_ADD_DRIVER (bumper2laser build_bumper2laser SOURCES bumper2laser.cc)

PLAYERDRIVER_OPTION (pbslaser build_pbslaser ON)
PLAYERDRIVER_ADD_DRIVER (pbslaser build_pbslaser SOURCES pbs_driver.cc)

PLAYERDRIVER_OPTION (sicklms200 build_sicklms200 ON)
PLAYERDRIVER_REQUIRE_HEADER (sicklms200 build_sicklms200 linux/serial.h)
PLAYERDRIVER_ADD_DRIVER (sicklms200 build_sicklms200 SOURCES sicklms200.cc)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (sicklms400 build_sicklms400 ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (sicklms400 build_sicklms400 OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_ADD_DRIVER (sicklms400 build_sicklms400 SOURCES lms400_cola.cc sicklms400.cc)

PLAYERDRIVER_OPTION (sicks3000 build_sicks3000 ON)
PLAYERDRIVER_REQUIRE_HEADER (sicks3000 build_sicks3000 linux/serial.h)
PLAYERDRIVER_ADD_DRIVER (sicks3000 build_sicks3000 SOURCES sicks3000.cc)

PLAYERDRIVER_OPTION (laserposeinterpolator build_laserposeinterpolator ON)
PLAYERDRIVER_ADD_DRIVER (laserposeinterpolator build_laserposeinterpolator SOURCES laserposeinterpolator.cc)

PLAYERDRIVER_OPTION (lasercspace build_lasercspace ON)
PLAYERDRIVER_ADD_DRIVER (lasercspace build_lasercspace SOURCES lasercspace.cc lasertransform.cc)

PLAYERDRIVER_OPTION (laserrescan build_laserrescan ON)
PLAYERDRIVER_ADD_DRIVER (laserrescan build_laserrescan SOURCES laserrescan.cc lasertransform.cc)

PLAYERDRIVER_OPTION (lasercutter build_lasercutter ON)
PLAYERDRIVER_ADD_DRIVER (lasercutter build_lasercutter SOURCES lasercutter.cc lasertransform.cc)

PLAYERDRIVER_OPTION (urglaser build_urglaser ON)
#PLAYERDRIVER_REQUIRE_HEADER (urglaser build_urglaser linux/serial.h)
PLAYERDRIVER_ADD_DRIVER (urglaser build_urglaser SOURCES urg_laser.cc urglaserdriver.cc)
# Also build and install standalone non-Player lib
PLAYER_ADD_LIBRARY (urglaser_standalone urg_laser.cc)
PLAYER_INSTALL_HEADERS (standalone_drivers urg_laser.h)


PLAYERDRIVER_OPTION (rs4leuze build_rs4leuze ON)
PLAYERDRIVER_ADD_DRIVER (rs4leuze build_rs4leuze SOURCES RS4LeuzeLaserDriver.cc RS4Leuze_laser.cc)
