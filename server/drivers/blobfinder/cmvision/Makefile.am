SUBDIRS = 
noinst_LIBRARIES = @CMVISION_LIB@ 

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -DUSE_METEOR @GAZEBO_EXTRA_CPPFLAGS@

EXTRA_LIBRARIES = libcmvision.a 

libcmvision_a_SOURCES = cmvision.cc cmvision.h conversions.c conversions.h \
	capture.h capture1394.cc capture1394.h \
	P2CMV.cc \
	captureV4L2.cc capturev4l.h capturev4l.cc captureV4L2.h \
	captureCamera.cc captureCamera.h
