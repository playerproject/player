AM_CPPFLAGS = -Wall -I@MOBILITY_DIR@/include -I@MOBILITY_DIR@/tools/include -I$(top_srcdir)/server -DUSE_MOBILITY -D__x86__ -D__linux__ -D__OSVERSION__=2 

noinst_LIBRARIES = @RWI_LIB@

EXTRA_LIBRARIES = librwi.a

librwi_a_SOURCES = rwi_bumperdevice.cc rwi_bumperdevice.h rwidevice.cc \
                   rwidevice.h rwi_laserdevice.cc rwi_laserdevice.h \
                   rwi_positiondevice.cc rwi_positiondevice.h \
                   rwi_powerdevice.cc rwi_powerdevice.h rwi_sonardevice.cc \
                   rwi_sonardevice.h
