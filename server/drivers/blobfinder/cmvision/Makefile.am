SUBDIRS = 
noinst_LIBRARIES = @CMVISION_LIB@ 

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -DUSE_METEOR @GAZEBO_EXTRA_CPPFLAGS@
          
EXTRA_LIBRARIES = libcmvision.a 

libcmvision_a_SOURCES = cmvision.cc conversions.c capture1394.cc \
                        captureGazebo.cc gz_camera.cc P2CMV.cc \
			captureV4L2.cc capturev4l.cc
                        
