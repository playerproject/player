if WITH_RWI
BUILDRWI = librwi.a
else
BUILDRWI = 
endif

AM_CPPFLAGS = -I$(top_srcdir)/server

noinst_LIBRARIES = $(BUILDRWI)

librwi_a_SOURCES = rwi_bumperdevice.cc rwi_bumperdevice.h rwidevice.cc \
                   rwidevice.h rwi_laserdevice.cc rwi_laserdevice.h \
                   rwi_positiondevice.cc rwi_positiondevice.h \
                   rwi_powerdevice.cc rwi_powerdevice.h rwi_sonardevice.cc \
                   rwi_sonardevice.h
clean-local:
	rm -f *.a
