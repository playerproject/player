# Automake script to build Stage's Makefile
#
# RTV 20021026
# $Id$

#SUBDIRS = rtkgui 

AM_CPPFLAGS = -DINCLUDE_RTK2 -Wall -I$(top_builddir)/replace -I@RTK_DIR@/include `gtk-config --cflags`  

noinst_LIBRARIES = @STAGE_LIB@
EXTRA_LIBRARIES = libstage.a


libstage_a_SOURCES = \
	colors.cc \
	colors.hh \
	entity.cc \
	entity.hh \
	gui.h \
	matrix.cc \
	matrix.hh \
	property.cc \
	raytrace.cc \
	raytrace.hh \
	raytrace.hh \
	root.cc \
	root.hh \
	rtkgui.hh \
	rtkgui.cc \
	stage.h \
	stage.cc \
	stagetime.cc \
	stagetime.hh \
	stg_puck.cc \
	stg_puck.hh \
	stg_position.cc \
	stg_position.hh \
	stg_idar.cc \
	stg_idar.hh \
	stg_sonar.cc \
	stg_sonar.hh

bin_PROGRAMS = stage
AM_CPPFLAGS += -I$(top_builddir)/client_libs/c++ -I$(top_builddir)/client_libs/c
LDADD = -L$(top_builddir)/client_libs/c++ -lplayerclient
stage_SOURCES = client.cc