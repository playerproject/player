PLAYERDRIVER_OPTION (isense build_isense ON)
PLAYERDRIVER_REQUIRE_HEADER (isense build_isense isense/isense.h)
PLAYERDRIVER_ADD_DRIVER (isense build_isense LINKFLAGS "-lisense" SOURCES intertiacube2.cc)
