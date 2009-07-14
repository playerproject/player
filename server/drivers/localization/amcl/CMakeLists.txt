PLAYERDRIVER_OPTION (amcl build_amcl ON)

SET (amclSrcs   amcl.cc
                amcl_sensor.cc
                amcl_odom.cc
                amcl_laser.cc
                models/odometry.c
                models/sonar.c
                models/laser.c
                models/wifi.c
                models/gps.c
                models/imu.c
                pf/pf.c
                pf/pf_kdtree.c
                pf/pf_pdf.c
                pf/pf_vector.c
                pf/pf_draw.c
                pf/eig3.c
                map/map.c
                map/map_range.c
                map/map_store.c
                map/map_draw.c)

IF (INCLUDE_RTKGUI)
    SET (linkFlags -L${PROJECT_BINARY_DIR}/rtk2 -lrtk ${GTK_LINKFLAGS})
    SET (cFlags -I${PROJECT_SOURCE_DIR}/rtk2 ${GTK_CFLAGS})
ELSE (INCLUDE_RTKGUI)
    SET (linkFlags)
    SET (cFlags)
ENDIF (INCLUDE_RTKGUI)

PLAYERDRIVER_ADD_DRIVER (amcl build_amcl LINKFLAGS ${linkFlags} CFLAGS ${cFlags} SOURCES ${amclSrcs})
