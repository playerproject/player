PLAYERDRIVER_OPTION (alsa build_alsa ON)
PLAYERDRIVER_REQUIRE_HEADER (alsa build_alsa alsa/asoundlib.h)
PLAYERDRIVER_ADD_DRIVER (alsa build_alsa LINKFLAGS "-lasound" SOURCES alsa.cc audio_sample.cc)