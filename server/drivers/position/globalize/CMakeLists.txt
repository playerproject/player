PLAYERDRIVER_OPTION (globalize build_globalize ON)
# This driver uses GlobalTime from libplayercore, which currently doesn't
# get exported correctly.
PLAYERDRIVER_REJECT_OS (globalize build_globalize PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (globalize build_globalize SOURCES globalize.cc)
