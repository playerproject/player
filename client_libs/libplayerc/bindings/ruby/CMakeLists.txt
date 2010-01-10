FIND_PACKAGE (Ruby)

IF (RUBY_LIBRARY AND RUBY_INCLUDE_PATH)
    SET (RUBY_FOUND TRUE)
ENDIF (RUBY_LIBRARY AND RUBY_INCLUDE_PATH)

IF(RUBY_FOUND)
    OPTION (BUILD_RUBY_BINDINGS "Build the Ruby bindings for the C client library" ON)

    IF (BUILD_RUBY_BINDINGS)
        FIND_PACKAGE (SWIG)
        IF (SWIG_FOUND)
            MESSAGE (STATUS "Ruby bindings for C client library will be built")

            #INCLUDE (${SWIG_USE_FILE})
            INCLUDE (${PLAYER_CMAKE_DIR}/internal/UseSwig_Player.cmake)

            INCLUDE_DIRECTORIES (${RUBY_INCLUDE_PATH})
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR})
            INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR})
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs/libplayerc)

            SET (CMAKE_SWIG_FLAGS "-w801")

            # Generate playerc_wrap.i and playerc_wrap.h from playerc.h
            SET (playerc_wrap_prefix "${CMAKE_CURRENT_BINARY_DIR}/playerc_wrap")
            ADD_CUSTOM_COMMAND (OUTPUT "${playerc_wrap_prefix}.i"
                COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../python/playerc_swig_parse.py ${PROJECT_SOURCE_DIR}/client_libs/libplayerc/playerc.h ${playerc_wrap_prefix}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                VERBATIM)

            ADD_CUSTOM_COMMAND(OUTPUT "${playerc_wrap_prefix}.h"
                DEPENDS "${playerc_wrap_prefix}.i")

            # Empty target to ensure playerc_wrap.i and playerc_wrap.h get created
            ADD_CUSTOM_TARGET (playerc_wrap_ri_target ALL
                DEPENDS "${playerc_wrap_prefix}.i"
                DEPENDS "${playerc_wrap_prefix}.h")

            SET (playerc_i "${CMAKE_CURRENT_SOURCE_DIR}/playerc.i")

            #If playercr is called simply playerc, all sorts of errors happen
            #I guess is namespace conflict with Python bindings' playerc
            #currently we create playercr locally but install it as playerc 
            SET (SWIG_MODULE_playercr_EXTRA_DEPS ${PROJECT_SOURCE_DIR}/client_libs/libplayerc/playerc.h
                                                 ${PROJECT_BINARY_DIR}/libplayerinterface/player_interfaces.h)
            SWIG_ADD_MODULE (playercr ruby ${playerc_i})
            SWIG_LINK_LIBRARIES (playercr ${RUBY_LIBRARY})
            ADD_DEPENDENCIES (${SWIG_MODULE_playercr_REAL_NAME} playerc_wrap_i_target)
            TARGET_LINK_LIBRARIES (${SWIG_MODULE_playercr_REAL_NAME} playerinterface playerc playercommon playerwkb)
            IF (HAVE_JPEG)
                TARGET_LINK_LIBRARIES (${SWIG_MODULE_playercr_REAL_NAME} playerjpeg)
            ENDIF (HAVE_JPEG)

            #TODO: find ruby in windows, check install dir is correct  
            STRING(REPLACE "/usr/" "" ruby_relative_sitearch_dir ${RUBY_SITEARCH_DIR})
            STRING(REPLACE "/opt/" "" ruby_relative_sitearch_dir ${ruby_relative_sitearch_dir})
            STRING(REPLACE "local/" "" ruby_relative_sitearch_dir ${ruby_relative_sitearch_dir})
	        STRING(REPLACE "lib/" ${PLAYER_LIBRARY_INSTALL_DIR}/ ruby_relative_sitearch_dir ${ruby_relative_sitearch_dir} )
            SET (RUBY_BINDINGS_INSTALL_DIR ${ruby_relative_sitearch_dir} CACHE PATH "Ruby bindings installation directory under $RUBY_sitearch_DIR")
            MARK_AS_ADVANCED (RUBY_BINDINGS_INSTALL_DIR)

            INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/playercr.so
                DESTINATION ${RUBY_BINDINGS_INSTALL_DIR}
                RENAME playerc.so)
            MESSAGE (STATUS "Ruby bindings for C client will be installed to ${RUBY_BINDINGS_INSTALL_DIR}")
        ELSE (SWIG_FOUND)
            MESSAGE (STATUS "Ruby bindings for C client library will not be built - could not find Swig")
        ENDIF (SWIG_FOUND)
    ELSE (BUILD_RUBY_BINDINGS)
        MESSAGE (STATUS "Ruby bindings for C client library will not be built - disabled by user")
    ENDIF (BUILD_RUBY_BINDINGS)
ELSE (RUBY_FOUND)
    MESSAGE (STATUS "Ruby bindings for C client library will not be built - could not find Ruby")
ENDIF (RUBY_FOUND)

