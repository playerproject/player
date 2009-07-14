PLAYERDRIVER_OPTION (bumper2laser build_bumper2laser ON)
PLAYERDRIVER_ADD_DRIVER (bumper2laser build_bumper2laser SOURCES bumper2laser.cc)

PLAYERDRIVER_OPTION (pbslaser build_pbslaser ON)
PLAYERDRIVER_REJECT_OS (pbslaser build_pbslaser PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (pbslaser build_pbslaser SOURCES pbs_driver.cc)

PLAYERDRIVER_OPTION (sicklms200 build_sicklms200 ON)
CHECK_INCLUDE_FILES (linux/serial.h HAVE_LINUX_SERIAL_H)
SET (laser_config_h_in ${CMAKE_CURRENT_SOURCE_DIR}/laser_config.h.in)
SET (laser_config_h ${CMAKE_CURRENT_BINARY_DIR}/laser_config.h)
CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/laser_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/laser_config.h)
PLAYERDRIVER_REJECT_OS (sicklms200 build_sicklms200 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (sicklms200 build_sicklms200 SOURCES sicklms200.cc ${laser_config_h}
                         INCLUDEDIRS ${CMAKE_CURRENT_BINARY_DIR})

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (sicklms400 build_sicklms400 ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (sicklms400 build_sicklms400 OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_REJECT_OS (sicklms400 build_sicklms400 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (sicklms400 build_sicklms400 SOURCES lms400_cola.cc sicklms400.cc)

PLAYERDRIVER_OPTION (sicks3000 build_sicks3000 ON)
PLAYERDRIVER_ADD_DRIVER (sicks3000 build_sicks3000 SOURCES sicks3000.cc)

PLAYERDRIVER_OPTION (laserposeinterpolator build_laserposeinterpolator ON)
PLAYERDRIVER_ADD_DRIVER (laserposeinterpolator build_laserposeinterpolator SOURCES laserposeinterpolator.cc)

PLAYERDRIVER_OPTION (lasercspace build_lasercspace ON)
PLAYERDRIVER_ADD_DRIVER (lasercspace build_lasercspace SOURCES lasercspace.cc lasertransform.cc)

PLAYERDRIVER_OPTION (laserrescan build_laserrescan ON)
PLAYERDRIVER_ADD_DRIVER (laserrescan build_laserrescan SOURCES laserrescan.cc lasertransform.cc)

PLAYERDRIVER_OPTION (lasercutter build_lasercutter ON)
PLAYERDRIVER_ADD_DRIVER (lasercutter build_lasercutter SOURCES lasercutter.cc lasertransform.cc)

PLAYERDRIVER_OPTION (rs4leuze build_rs4leuze ON)
PLAYERDRIVER_REJECT_OS (rs4leuze build_rs4leuze PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (rs4leuze build_rs4leuze SOURCES RS4LeuzeLaserDriver.cc RS4Leuze_laser.cc)

PLAYERDRIVER_OPTION (sickLDMRS build_sickLDMRS ON)
PLAYERDRIVER_ADD_DRIVER (sickLDMRS build_sickLDMRS SOURCES sickLDMRS.cc)

