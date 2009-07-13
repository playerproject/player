OPTION (BUILD_DOCUMENTATION "Build the doxygen-based documentation" OFF)
IF (BUILD_DOCUMENTATION)
    MESSAGE (STATUS)
    MESSAGE (STATUS "===== Documentation =====")
    FIND_PACKAGE (Doxygen)
    IF (DOXYGEN_FOUND)
        SET (doxygenConfigFileIn "${CMAKE_CURRENT_SOURCE_DIR}/player.dox.in")
        SET (doxygenConfigFile "${CMAKE_CURRENT_BINARY_DIR}/player.dox")

        SET (DOXYGEN_LANGUAGE "English" CACHE STRING "Documentation language")
        MARK_AS_ADVANCED (DOXYGEN_LANGUAGE)

        IF (PLAYER_OS_WIN)
            FIND_PACKAGE (HTMLHelp)
            IF (HTML_HELP_COMPILER)
                SET (DOXYGEN_HTMLHELP YES)
            ELSE (HTML_HELP_COMPILER)
                SET (DOXYGEN_HTMLHELP NO)
            ENDIF (HTML_HELP_COMPILER)
        ELSE (PLAYER_OS_WIN)
            SET (DOXYGEN_HTMLHELP NO)
        ENDIF (PLAYER_OS_WIN)

        FIND_PACKAGE (LATEX)
        IF (LATEX_COMPILER OR PDFLATEX_COMPILER)
            SET (DOXYGEN_LATEX YES)
            IF (PDFLATEX_COMPILER)
                SET (DOXYGEN_PDFLATEX YES)
            ELSE (PDFLATEX_COMPILER)
                SET (DOXYGEN_PDFLATEX NO)
            ENDIF (PDFLATEX_COMPILER)
        ELSE (LATEX_COMPILER OR PDFLATEX_COMPILER)
            SET (DOXYGEN_LATEX NO)
            SET (DOXYGEN_PDFLATEX NO)
        ENDIF (LATEX_COMPILER OR PDFLATEX_COMPILER)

        FIND_PROGRAM (HAVE_DOT dot)
        IF (HAVE_DOT)
            SET (HAVE_DOT_PROGRAM YES)
        ENDIF (HAVE_DOT)
        FIND_PACKAGE (Perl)

        # Generate the doxygen configure file based on various configuration parameters
        CONFIGURE_FILE (${doxygenConfigFileIn} ${doxygenConfigFile} @ONLY)

        # Build documentation target
        ADD_CUSTOM_TARGET (doc
            ${DOXYGEN_EXECUTABLE} ${doxygenConfigFile}
            DEPENDS ${doxygenConfigFile}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)
        ADD_DEPENDENCIES (doc playercore playerinterface)

        # Install documentation
        SET (docInstallDir "share/doc/player-${PLAYER_VERSION}")
        SET (docSourceDir "${CMAKE_CURRENT_BINARY_DIR}/player-docs/")
        # Make the output directory now so install doesn't complain that it doesn't exist
        FILE (MAKE_DIRECTORY ${docSourceDir})
        INSTALL (DIRECTORY ${docSourceDir}
            DESTINATION ${docInstallDir})

        # Compile the LaTeX documentation
        IF (DOXYGEN_LATEX)
            INCLUDE (${PLAYER_CMAKE_DIR}/internal/CompileLatex.cmake)
            COMPILE_LATEX (${CMAKE_CURRENT_BINARY_DIR}/latex refman)
            ADD_DEPENDENCIES (doc_latex doc)
        ENDIF (DOXYGEN_LATEX)

        # Upload documentation target (not supported on Windows)
        SET (docWebDestination "Player-cvs")
        SET (docName "player")
        IF (NOT PLAYER_OS_WIN)
            ADD_CUSTOM_TARGET (upload_doc
                ${CMAKE_CURRENT_SOURCE_DIR}/upload_www.sh ${docWebDestination} ${docName}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                VERBATIM)
            ADD_DEPENDENCIES (upload_doc doc)
        ENDIF (NOT PLAYER_OS_WIN)
    ELSE (DOXYGEN_FOUND)
        MESSAGE (STATUS "Documentation will not be built - Doxygen not found")
    ENDIF (DOXYGEN_FOUND)
ENDIF (BUILD_DOCUMENTATION)