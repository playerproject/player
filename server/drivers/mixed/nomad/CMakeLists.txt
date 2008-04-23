PLAYERDRIVER_OPTION (nomad build_nomad OFF "Disabled by default")
SET (NOMAD_INTERFACE "Nclient.c" CACHE STRING "Choose a Nomadics interface. The default is for connection via Nserver simulator/visualisation")
MARK_AS_ADVANCED (NOMAD_INTERFACE)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (nomad build_nomad nomad.cc nomad_position.cc nomad_sonar.cc ${NOMAD_INTERFACE})