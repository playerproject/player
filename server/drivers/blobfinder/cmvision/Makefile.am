SUBDIRS = 
noinst_LIBRARIES = @CMVISION_LIB@ 

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -DUSE_METEOR

EXTRA_LIBRARIES = libcmvision.a 

libcmvision_a_SOURCES = cmvision.cc cmvision.h conversions.c conversions.h \
	capture.h capture1394.cc capture1394.h \
	P2CMV.cc capturev4l.h capturev4l.cc \
	captureCamera.cc captureCamera.h

# V4L2 support is disabled, until it's fixed.
EXTRA_DIST = captureV4L2.h  captureV4L2.cc 
