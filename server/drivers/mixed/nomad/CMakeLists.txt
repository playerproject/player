PLAYERDRIVER_OPTION (nomad build_nomad ON)
PLAYERDRIVER_REJECT_OS (nomad build_nomad PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (nomad build_nomad SOURCES nomad.cc Nclient.c)
