SUBDIRS = 
noinst_LIBRARIES = @CMVISION_LIB@ 

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server -DUSE_METEOR

EXTRA_LIBRARIES = libcmvision.a 

libcmvision_a_SOURCES = cmvision.cc cmvision.h conversions.c conversions.h \
	capture.h P2CMV.cc captureCamera.cc captureCamera.h
