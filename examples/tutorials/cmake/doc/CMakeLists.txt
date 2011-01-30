OPTION (BUILD_DOCUMENTATION "Build the doxygen-based documentation" OFF)
IF (BUILD_DOCUMENTATION)

    # Make sure Doxygen is on the system, if not then the documentation can't be built
    FIND_PACKAGE (Doxygen)
    IF (DOXYGEN_FOUND)

        # This is the doxyfile that will be used to generate the documentation
        # You can use programs like doxywizard to edit the settings
        SET (doxygenConfigFileIn "${CMAKE_CURRENT_SOURCE_DIR}/example.dox.in")
        SET (doxygenConfigFile "${CMAKE_CURRENT_BINARY_DIR}/example.dox")

        SET (DOXYGEN_LANGUAGE "English" CACHE STRING "Documentation language")
        MARK_AS_ADVANCED (DOXYGEN_LANGUAGE)

        # Using a .in file means we can use CMake to insert project settings
        # into the doxyfile.  For example, CMake will replace @PROJECT_NAME@ in
        # a configured file with the CMake PROJECT_NAME variable's value.
        CONFIGURE_FILE (${doxygenConfigFileIn} ${doxygenConfigFile} @ONLY)

        # Add the documentiation target.  This lets you run "make doc" from the
        # generated CMake makefiles
        ADD_CUSTOM_TARGET (doc
            ${DOXYGEN_EXECUTABLE} ${doxygenConfigFile}
            DEPENDS ${doxygenConfigFile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)

        # You can add an "install" directive to install the resulting documentation
        # if desired.
    ELSE (DOXYGEN_FOUND)
        MESSAGE (STATUS "Documentation will not be built - Doxygen not found")
    ENDIF (DOXYGEN_FOUND)
ENDIF (BUILD_DOCUMENTATION)
