noinst_LIBRARIES = @LOCALIZATION_LIB@ 

AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

EXTRA_LIBRARIES = liblocalization.a

liblocalization_a_SOURCES = regular_mcl.cc \
		            world_model.cc \
		            sensor_model.cc \
		            action_model.cc \
			    clustering.cc \
                            action_model.h \
                            clustering.h \
                            mcl_types.h \
                            regular_mcl.h \
                            sensor_model.h \
                            world_model.h
