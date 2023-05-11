find_package(Doxygen)
find_program(DOT dot)
if(DOXYGEN_FOUND AND DOT)
  message(STATUS "Using doxygen: ${DOXYGEN_VERSION} (with ${DOT})")
  find_package(Java COMPONENTS Runtime)
  if(Java_Runtime_FOUND)
    include(UseJava)
    find_jar(PLANTUML_JAR_PATH NAMES plantuml)
    message(STATUS "Found plantuml: ${PLANTUML_JAR_PATH}")
  endif()
  file(
    GLOB_RECURSE
    COUCHBASE_CXX_CLIENT_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/couchbase/**/*.hxx
    ${PROJECT_SOURCE_DIR}/docs/*.hxx
    ${PROJECT_SOURCE_DIR}/docs/*.md)

  set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/couchbase)
  set(DOXYGEN_OUTPUT_DIR ${PROJECT_BINARY_DIR}/couchbase-cxx-client-${COUCHBASE_CXX_CLIENT_SEMVER})
  set(DOXYGEN_INDEX_HTML_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
  set(DOXYGEN_INDEX_XML_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
  set(DOXYGEN_CONFIG_TEMPLATE ${PROJECT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYGEN_CONFIG ${PROJECT_BINARY_DIR}/Doxyfile)
  configure_file(${DOXYGEN_CONFIG_TEMPLATE} ${DOXYGEN_CONFIG})
  add_custom_command(
    OUTPUT ${DOXYGEN_INDEX_HTML_FILE} ${DOXYGEN_INDEX_XML_FILE}
    DEPENDS ${COUCHBASE_CXX_CLIENT_PUBLIC_HEADERS}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    WORKING_DIRECTORY ${DOXYGEN_OUTPUT_DIR}
    MAIN_DEPENDENCY ${DOXYGEN_CONFIG}
    ${DOXYGEN_CONFIG_TEMPLATE}
    COMMENT "Generating documentation with Doxygen: ${DOXYGEN_INDEX_HTML_FILE}")
  add_custom_target(doxygen DEPENDS ${DOXYGEN_INDEX_HTML_FILE} ${COUCHBASE_CXX_CLIENT_PUBLIC_HEADERS})

  find_program(SPHINX_BUILD sphinx-build)
  if(SPHINX_BUILD)
    set(SPHINX_SOURCE_DIR ${PROJECT_BINARY_DIR}/source)
    set(SPHINX_BUILD_DIR ${DOXYGEN_OUTPUT_DIR}/sphinx)
    set(SPHINX_CONF_PY ${SPHINX_SOURCE_DIR}/conf.py)
    set(SPHINX_INDEX_RST ${SPHINX_SOURCE_DIR}/index.rst)

    configure_file(${PROJECT_SOURCE_DIR}/docs/source/conf.py.in ${SPHINX_CONF_PY})
    configure_file(${PROJECT_SOURCE_DIR}/docs/source/index.rst.in ${SPHINX_INDEX_RST})
    file(MAKE_DIRECTORY ${SPHINX_SOURCE_DIR}/_static)

    add_custom_command(
            OUTPUT ${SPHINX_BUILD_DIR}/index.html
            DEPENDS ${DOXYGEN_INDEX_XML_FILE}
            COMMAND ${SPHINX_BUILD} ${SPHINX_SOURCE_DIR} ${SPHINX_BUILD_DIR}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            MAIN_DEPENDENCY ${SPHINX_CONF_PY} ${SPHINX_INDEX_RST}
            COMMENT "Generating documentation with Sphinx")

    add_custom_target(sphinx DEPENDS ${SPHINX_BUILD_DIR}/index.html ${SPHINX_CONF_PY} ${SPHINX_INDEX_RST})
  endif ()
else()
  message(STATUS "Could not find doxygen executable. Documentation generation will be disabled.")
endif()
