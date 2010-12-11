if (NOT HAVE_STL)
    PLAYERDRIVER_OPTION (differential build_differential OFF "STL not found")
else(NOT HAVE_STL)
    PLAYERDRIVER_OPTION (differential build_differential ON)
endif(NOT HAVE_STL)

PLAYERDRIVER_ADD_DRIVER(
    differential build_differential
    
    SOURCES
    chronos.cc
    driver.cc
    )
