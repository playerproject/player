AM_CPPFLAGS = -Wall -I$(top_srcdir)/server

noinst_LIBRARIES = @NOMAD_LIB@ 

EXTRA_LIBRARIES = libnomad.a

# for direct connection to a robot
# libnomad_a_SOURCES = nomad.cc direct.c Nclient.h mother.cc

# for connection via Nserver (good for debugging)
libnomad_a_SOURCES = \
  nomad.cc \
  Nclient.c \
  Nclient.h \
  nomad_position.cc
