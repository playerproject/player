###########################################################################
# Desc: Makefile for local map
# Author: Andrew Howard
# Date: 18 Jan 2003
# CVS: $Id$
###########################################################################


LIB = ../libimap.a
PY_LIB = imap.so
PY_OBS = pyimap.o ../imap_draw.o

IMAP_CFLAGS = -I. -I../
IMAP_LFLAGS = -L../ -limap

RTK_CFLAGS = -I$(HOME)/include `gtk-config --cflags`
RTK_LFLAGS = -L$(HOME)/lib -lrtk -ljpeg `gtk-config --libs gtk gthread`

GSL_CFLAGS = 
GSL_LFLAGS = -lgsl -lgslcblas

#IM_CFLAGS = `Magick-config --cflags --cppflags`
#IM_LFLAGS = `Magick-config --ldflags --libs`

PYTHON_FLAGS = -I/usr/include/python2.2 -I/usr/local/include/python2.2

CFLAGS = -g3 -O2 -Wall -fPIC $(IMAP_CFLAGS) $(PYTHON_FLAGS) $(RTK_CFLAGS)
LFLAGS = -shared $(IMAP_LFLAGS) $(GSL_LFLAGS) $(IM_LFLAGS) $(RTK_LFLAGS)

MAKEDEP = makedepend



###########################################################################
# Build section

all: $(PY_LIB)

$(PY_LIB) : $(PY_OBS) $(LIB)
	$(CC) $(PY_OBS) $(LFLAGS) -o $(PY_LIB)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o *.so
