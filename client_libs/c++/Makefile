#
# $Id$
#

include ../../Makefile.common

LIBNAME = libplayerclient.a

CFLAGS = -g -O2 -Wall $(PLAYER_PLATFORM)
INCLUDES = -I. -I../c -I../../include

OBS = playerclient.o clientproxy.o laserproxy.o sonarproxy.o miscproxy.o \
      positionproxy.o ptzproxy.o gripperproxy.o speechproxy.o \
      laserbeaconproxy.o visionproxy.o

all: $(LIBNAME) 

dep:
	$(MAKEDEP) $(INCLUDES) -Y -s "# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND" *.cc 2>/dev/null
	$(RM) -f Makefile.bak

test: test.cc $(LIBNAME)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ $^

$(LIBNAME): $(OBS) ../c/playercclient.o
	ar cr $@ $^

%.o: %.cc
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c $<

clean:
	$(RM) -f $(LIBNAME) *.o test
# DO NOT DELETE.  FOLLOWING IS FROM MAKEDEPEND
