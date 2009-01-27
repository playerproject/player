PLAYERDRIVER_OPTION (alsa build_alsa ON)
PLAYERDRIVER_REJECT_OS (alsa build_alsa PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (alsa build_alsa alsa/asoundlib.h)
PLAYERDRIVER_ADD_DRIVER (alsa build_alsa LINKFLAGS "-lasound" SOURCES alsa.cc audio_sample.cc)
