option (PARAMETRIC_BUILD_DOCS "Build documentation (requires doxygen)" OFF)

if (PARAMETRIC_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    find_package(Sphinx REQUIRED)

    #This will be the main output of our command
    set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/html/index.html)

    set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/parametric)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    #Replace variables inside @@ with the current values
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR}) #Doxygen won't create this for us
    add_custom_command(OUTPUT documentation
                       COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                       MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                       COMMENT "Generating docs")

    add_custom_target(Doxygen ALL DEPENDS documentation)

    set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
    set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

    add_custom_target(Sphinx ALL
                      DEPENDS Doxygen
                      COMMAND
                      ${SPHINX_EXECUTABLE} -b html
                      -Dbreathe_projects.parametric=${DOXYGEN_OUTPUT_DIR}/xml
                      ${SPHINX_SOURCE} ${SPHINX_BUILD}
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating documentation with Sphinx")

endif(PARAMETRIC_BUILD_DOCS)
