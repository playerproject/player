
CXX = @CXX_SERVER@
AR = @AR_SERVER@
RANLIB = @RANLIB_SERVER@
CXXFLAGS = @CXXFLAGS_SERVER@


AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

noinst_LIBRARIES = @REB_LIB@

EXTRA_LIBRARIES = libreb.a

libreb_a_SOURCES = reb.cc reb_position.cc reb_ir.cc reb_power.cc reb_params.cc \
			reb_params.h
