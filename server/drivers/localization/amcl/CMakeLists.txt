PLAYERDRIVER_OPTION (amcl build_amcl ON)

PLAYERDRIVER_REQUIRE_HEADER (amcl build_amcl gsl/gsl_version.h)

SET (amclSrcs   amcl.h
                amcl.cc
                amcl_sensor.h
                amcl_sensor.cc
                amcl_odom.h
                amcl_odom.cc
                amcl_laser.h
                amcl_laser.cc
                models/odometry.h
                models/odometry.c
                models/sonar.h
                models/sonar.c
                models/laser.h
                models/laser.c
                models/wifi.h
                models/wifi.c
                models/gps.h
                models/gps.c
                models/imu.h
                models/imu.c
                pf/pf.c
                pf/pf_kdtree.c
                pf/pf_pdf.c
                pf/pf_vector.c
                pf/pf_draw.c
                pf/pf.h
                pf/pf_pdf.h
                pf/pf_kdtree.h
                pf/pf_vector.h
                map/map.c
                map/map_range.c
                map/map_store.c
                map/map_draw.c
                map/map.h)

IF (INCLUDE_RTKGUI)
    SET (linkFlags "-lgsl -lgslcblas ${GTK_LINKFLAGS}")
    SET (cFlags "-I${PROJECT_SOURCE_DIR}/rtk2 ${GTK_CFLAGS} -DINCLUDE_RTKGUI")
ELSE (INCLUDE_RTKGUI)
    SET (linkFlags "-lgsl -lgslcblas")
    SET (cFlags "")
ENDIF (INCLUDE_RTKGUI)

PLAYERDRIVER_ADD_DRIVER (amcl build_amcl "" "" ${linkFlags} "${cFlags}" ${amclSrcs})