PLAYERDRIVER_OPTION (rflex build_rflex ON)
PLAYERDRIVER_REJECT_OS (rflex build_rflex PLAYER_OS_WIN)
PLAYERDRIVER_REJECT_OS (reflex build_rflex PLAYER_OS_SOLARIS)
PLAYERDRIVER_ADD_DRIVER (rflex build_rflex SOURCES rflex.cc rflex_commands.cc rflex-io.cc)
