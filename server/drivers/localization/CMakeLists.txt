ADD_SUBDIRECTORY (amcl)

PLAYERDRIVER_OPTION (fakelocalize build_fakelocalize ON)
PLAYERDRIVER_ADD_DRIVER (fakelocalize build_fakelocalize SOURCES fakelocalize.cc)
