PLAYERDRIVER_OPTION (bumper2laser build_bumper2laser ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (bumper2laser build_bumper2laser bumper2laser.cc)

PLAYERDRIVER_OPTION (pbslaser build_pbslaser ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (pbslaser build_pbslaser pbs_driver.cc)

PLAYERDRIVER_OPTION (sicklms200 build_sicklms200 ON)
PLAYERDRIVER_REQUIRE_HEADER (sicklms200 build_sicklms200 linux/serial.h)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (sicklms200 build_sicklms200 sicklms200.cc)

PLAYERDRIVER_OPTION (sicklms400 build_sicklms400 ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (sicklms400 build_sicklms400 lms400_cola.cc sicklms400.cc)

PLAYERDRIVER_OPTION (sicks3000 build_sicks3000 ON)
PLAYERDRIVER_REQUIRE_HEADER (sicks3000 build_sicks3000 linux/serial.h)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (sicks3000 build_sicks3000 sicks3000.cc)

PLAYERDRIVER_OPTION (laserposeinterpolator build_laserposeinterpolator ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (laserposeinterpolator build_laserposeinterpolator laserposeinterpolator.cc)

PLAYERDRIVER_OPTION (lasercspace build_lasercspace ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (lasercspace build_lasercspace lasercspace.cc lasertransform.cc)

PLAYERDRIVER_OPTION (laserrescan build_laserrescan ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (laserrescan build_laserrescan laserrescan.cc lasertransform.cc)

PLAYERDRIVER_OPTION (lasercutter build_lasercutter ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (lasercutter build_lasercutter lasercutter.cc lasertransform.cc)

PLAYERDRIVER_OPTION (urglaser build_urglaser ON)
#PLAYERDRIVER_REQUIRE_HEADER (urglaser build_urglaser linux/serial.h)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (urglaser build_urglaser urg_laser.cc urglaserdriver.cc)
# Also build and install standalone non-Player lib
PLAYER_ADD_LIBRARY (urglaser_standalone urg_laser.cc)
PLAYER_INSTALL_HEADERS (standalone_drivers urg_laser.h)


PLAYERDRIVER_OPTION (rs4leuze build_rs4leuze ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (rs4leuze build_rs4leuze RS4LeuzeLaserDriver.cc RS4Leuze_laser.cc)
