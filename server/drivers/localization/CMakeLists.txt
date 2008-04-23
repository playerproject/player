ADD_SUBDIRECTORY (amcl)

PLAYERDRIVER_OPTION (fakelocalize build_fakelocalize ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (fakelocalize build_fakelocalize fakelocalize.cc)