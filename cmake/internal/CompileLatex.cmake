MACRO (COMPILE_LATEX _workingDir _docName)
    FIND_PACKAGE (LATEX)

    SET (texFile ${_workingDir}/${_docName}.tex)
    SET (auxFile ${_workingDir}/${_docName}.aux)
    SET (logFile ${_workingDir}/${_docName}.log)
    SET (idxFile ${_workingDir}/${_docName}.idx)
    SET (indFile ${_workingDir}/${_docName}.ind)
#     SET (dviFile ${_workingDir}/${_docName}.dvi)
    SET (pdfFile ${_workingDir}/${_docName}.pdf)

    ADD_CUSTOM_COMMAND (OUTPUT ${idxFile}
        DEPENDS ${texFile}
        COMMAND ${PDFLATEX_COMPILER}
        ARGS -interaction=batchmode ${_docName}.tex
        WORKING_DIRECTORY ${_workingDir}
        COMMENT "PDFLatex (first pass)")

    ADD_CUSTOM_COMMAND (OUTPUT ${indFile}
        DEPENDS ${idxFile}
        COMMAND ${MAKEINDEX_COMPILER}
        ARGS ${_docName}.idx
        WORKING_DIRECTORY ${_workingDir}
        COMMENT "Make index")

    ADD_CUSTOM_COMMAND (OUTPUT ${pdfFile}
        DEPENDS ${indFile}
        COMMAND ${PDFLATEX_COMPILER}
        ARGS -interaction=batchmode ${_docName}.tex
        WORKING_DIRECTORY ${_workingDir}
        COMMENT "PDFLatex (second pass)")

    ADD_CUSTOM_TARGET (doc_latex
        DEPENDS ${pdfFile})
ENDMACRO (COMPILE_LATEX)
