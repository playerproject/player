AM_CPPFLAGS = -Wall -I$(top_srcdir)/server @GAZEBO_CFLAGS@


#noinst_LTLIBRARIES = @GAZEBO_LIB@

noinst_LTLIBRARIES = 
if INCLUDE_GAZEBO
noinst_LTLIBRARIES += libgazebo.la
endif

libgazebo_la_SOURCES = gz_client.h gz_client.cc gz_time.h gz_time.cc gz_sim.cc gz_camera.cc gz_factory.cc gz_fiducial.cc gz_gps.cc gz_laser.cc gz_position.cc gz_power.cc gz_ptz.cc gz_truth.cc gz_position3d.cc gz_gripper.cc gz_sonar.cc gz_stereo.cc 

include_HEADERS = 
