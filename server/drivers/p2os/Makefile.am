if WITH_P2OS
BUILDP2OS = libp2os.a
else
BUILDP2OS = 
endif

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = $(BUILDP2OS)

libp2os_a_SOURCES = p2os.cc p2os_gripper.cc p2os.h p2os_misc.cc \
                    p2os_position.cc p2os_sonar.cc packet.cc packet.h \
                    robot_params.cc robot_params.h sip.cc sip.h

clean-local:
	rm -f *.a
