###########################################################################
#
# File: Makefile
# Author: Andrew Howard
# Date: 21 Mar 2001
# Desc:
#
# CVS info:
#  $Source$
#  $Author$
#  $Revision$
#
# Usage:
#  (empty)
#
# Theory of operation:
#  (empty)
#
# Known bugs:
#  (empty)
#
# Possible enhancements:
#  (empty)
#
###########################################################################

.SUFFIXES: .c .o

# Paths
PYTHON_INC = -I/usr/local/include/python2.1
PLAYER_INC = -I../../server -I../libplayerc
PLAYER_LIB = ../libplayerc

DEFINES = -DHAVE_SYS_TYPES_H=1 -DHAVE_STDINT_H=1

LIB = playerc.so
OBS = pyplayerc.o \
			dev_laser.o \
			dev_position.o \
			dev_ptz.o \
			dev_vision.o

CFLAGS = -g3 -Wall -fPIC $(DEFINES) $(PYTHON_INC) $(PLAYER_INC)
LFLAGS = -shared -lm -L$(PLAYER_LIB) -lplayerc


###########################################################################
# Build section

all: $(LIB)

$(LIB) : $(OBS)
	gcc $(OBS) $(LFLAGS) -o $(LIB)

%.o : %.c
	${CC} ${CFLAGS} -c $< -o $@

dep:
	$(MAKEDEP) -I../include -Y -s "# Dependancies (make dep)" *.cc 2>/dev/null

clean:
	rm -f *~ *.o


# Dependancies (make dep)
