SET (sicknav200Srcs sicknav200.cc nav200.cc)
PLAYERDRIVER_OPTION (sicknav200 build_sicknav200 ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (sicknav200 build_sicknav200 ${sicknav200Srcs})

ADD_EXECUTABLE (test_program_init testprogram_init.cpp ${sicknav200Srcs})
ADD_EXECUTABLE (test_program test_program.cpp ${sicknav200Srcs})
ADD_EXECUTABLE (test test.cpp ${sicknav200Srcs})