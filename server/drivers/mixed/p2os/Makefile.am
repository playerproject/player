AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

noinst_LIBRARIES = @P2OS_LIB@ 

EXTRA_LIBRARIES = libp2os.a

libp2os_a_SOURCES = p2os.cc p2os.h \
                    packet.h packet.cc \
		    robot_params.cc robot_params.h \
		    sip.cc sip.h

#p2os_gripper.cc p2os_bumper.cc 
#p2os_power.cc p2os_aio.cc p2os_cmucam.cc p2os_dio.cc
#p2os_position.cc p2os_sonar.cc p2os_sound.cc 
#p2os_compass.cc p2os_gyro.cc
