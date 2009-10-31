PLAYERDRIVER_OPTION (amtecpowercube build_amtecpowercube ON)
PLAYERDRIVER_REJECT_OS (amtecpowercube build_amtecpowercube PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (amtecpowercube build_amtecpowercube SOURCES amtecpowercube.cc)

PLAYERDRIVER_OPTION (canonvcc4 build_canonvcc4 ON)
PLAYERDRIVER_REJECT_OS (canonvcc4 build_canonvcc4 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (canonvcc4 build_canonvcc4 SOURCES canonvcc4.cc)

PLAYERDRIVER_OPTION (ptu46 build_ptu46 ON)
PLAYERDRIVER_REJECT_OS (ptu46 build_ptu46 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (ptu46 build_ptu46 SOURCES ptu46.cc)

PLAYERDRIVER_OPTION (sonyevid30 build_sonyevid30 ON)
PLAYERDRIVER_REJECT_OS (sonyevid30 build_sonyevid30 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (sonyevid30 build_sonyevid30 SOURCES sonyevid30.cc)

ADD_SUBDIRECTORY (sphereptz)
