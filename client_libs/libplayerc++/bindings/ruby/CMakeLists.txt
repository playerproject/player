FIND_PACKAGE (Ruby)

IF (RUBY_LIBRARY AND RUBY_INCLUDE_PATH)
    SET (RUBY_FOUND TRUE)
ENDIF (RUBY_LIBRARY AND RUBY_INCLUDE_PATH)

IF(BUILD_PLAYERCC)
    IF(RUBY_FOUND)
        OPTION (BUILD_RUBY_CPP_BINDINGS "Build the Ruby bindings for the C++ client library" ON)

        IF (BUILD_RUBY_CPP_BINDINGS)
            FIND_PACKAGE (SWIG)
            IF (SWIG_FOUND)
                MESSAGE (STATUS "Ruby bindings for C++ client library will be built")

                INCLUDE (${PLAYER_CMAKE_DIR}/internal/UseSwig_Player.cmake)

                INCLUDE_DIRECTORIES (${RUBY_INCLUDE_PATH})
                INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR})
                INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR})
                INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)
                INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR}/client_libs)

                SET (CMAKE_SWIG_FLAGS "-w801")

                SET (playercpp_ruby_i "${CMAKE_CURRENT_SOURCE_DIR}/playercpp.i")
                SET_SOURCE_FILES_PROPERTIES (${playercpp_ruby_i} PROPERTIES CPLUSPLUS ON)

                SET (SWIG_MODULE_playercppr_EXTRA_DEPS ${PROJECT_SOURCE_DIR}/client_libs/libplayerc++/playerc++.h)
                SWIG_ADD_MODULE (playercppr ruby ${playercpp_ruby_i})
                SWIG_LINK_LIBRARIES (playercppr ${RUBY_LIBRARY} playerc++)

                # TODO: find ruby in windows, check install dir is correct  
                STRING(REPLACE "/usr/" "" ruby_relative_sitearch_dir ${RUBY_SITEARCH_DIR})
                STRING(REPLACE "/opt/" "" ruby_relative_sitearch_dir ${ruby_relative_sitearch_dir})
                STRING(REPLACE "local/" "" ruby_relative_sitearch_dir ${ruby_relative_sitearch_dir})
                STRING(REPLACE "lib/" ${PLAYER_LIBRARY_INSTALL_DIR}/ ruby_relative_sitearch_dir ${ruby_relative_sitearch_dir})
                SET (RUBY_BINDINGS_INSTALL_DIR ${ruby_relative_sitearch_dir} CACHE PATH "Ruby bindings for the c++ library installation directory under $RUBY_SITEARCH_DIR")
                MARK_AS_ADVANCED (RUBY_BINDINGS_INSTALL_DIR)

                INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/playercppr.so
                    DESTINATION ${RUBY_BINDINGS_INSTALL_DIR}
                    RENAME playercpp.so)
            ELSE (SWIG_FOUND)
                MESSAGE (STATUS "Ruby bindings for C++ client library will not be built - could not find Swig")
            ENDIF (SWIG_FOUND)
        ELSE (BUILD_RUBY_CPP_BINDINGS)
            MESSAGE (STATUS "Ruby bindings for C++ client library will not be built - disabled by user")
        ENDIF (BUILD_RUBY_CPP_BINDINGS)
    ELSE (RUBY_FOUND)
        MESSAGE (STATUS "Ruby bindings for C++ client library will not be built - could not find Ruby")
    ENDIF (RUBY_FOUND)
ELSE (BUILD_PLAYERCC)
  MESSAGE (STATUS "Ruby bindings for C++ client library will not be built - playerc++ client library disabled")
ENDIF (BUILD_PLAYERCC)
