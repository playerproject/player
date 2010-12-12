ADD_SUBDIRECTORY (amcl)
ADD_SUBDIRECTORY (ekfvloc)

PLAYERDRIVER_OPTION (fakelocalize build_fakelocalize ON)
PLAYERDRIVER_ADD_DRIVER (fakelocalize build_fakelocalize SOURCES fakelocalize.cc)
