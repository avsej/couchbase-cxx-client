if(NOT TARGET CLI11::CLI11)
  # https://github.com/CLIUtils/CLI11/releases
  cpmaddpackage(
    NAME
    cli11
    VERSION
    2.5.0
    GITHUB_REPOSITORY
    "cliutils/cli11"
    OPTIONS
    "EXCLUDE_FROM_ALL ON"
    "BUILD_TESTING OFF"
    "CLI11_BUILD_TESTS OFF"
    "CLI11_BUILD_DOCS OFF"
    "CLI11_BUILD_EXAMPLES OFF"
    "CLI11_SINGLE_FILE_TESTS OFF"
    "CLI11_INSTALL OFF"
    "CLI11_PRECOMPILED ON")
endif()

set(cbc_SOURCES
  cbc.cxx
  utils.cxx
  key_generator.cxx
  analytics.cxx
  beam.cxx
  get.cxx
  keygen.cxx
  pillowfight.cxx
  query.cxx
  remove.cxx
  upsert.cxx
  version.cxx
  config.cxx)

cpmaddpackage(
  NAME
  imgui
  VERSION
  1.92.4
  GITHUB_REPOSITORY
  ocornut/imgui
  DOWNLOAD_ONLY
  TRUE)

configure_file(${PROJECT_SOURCE_DIR}/cmake/Dear_ImGui.cmake "${imgui_SOURCE_DIR}/CMakeLists.txt" COPYONLY)

set(IMGUI_EXAMPLES FALSE)
set(IMGUI_DEMO FALSE)
set(IMGUI_ENABLE_STDLIB_SUPPORT TRUE)
# # FreeType (https://github.com/cpm-cmake/CPM.cmake/wiki/More-Snippets#freetype)
# set(FREETYPE_FOUND TRUE)
# set(FREETYPE_INCLUDE_DIRS "")
# set(FREETYPE_LIBRARIES Freetype::Freetype)

add_subdirectory(${imgui_SOURCE_DIR} ${imgui_BINARY_DIR} EXCLUDE_FROM_ALL SYSTEM)
list(APPEND cbc_SOURCES config-gui.cxx)

add_executable(cbc ${cbc_SOURCES})
target_include_directories(cbc PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/private)

set_project_warnings(cbc)
set_project_options(cbc)

target_link_libraries(
  cbc
  PRIVATE ${CMAKE_THREAD_LIBS_INIT}
          ${couchbase_cxx_client_DEFAULT_LIBRARY}
          $<BUILD_INTERFACE:CLI11>
          $<BUILD_INTERFACE:Microsoft.GSL::GSL>
          $<BUILD_INTERFACE:taocpp::json>
          $<BUILD_INTERFACE:spdlog::spdlog>
          $<BUILD_INTERFACE:hdr_histogram_static>
          $<BUILD_INTERFACE:asio>)
propagate_public_compile_definitions(cbc spdlog::spdlog asio)

target_link_libraries(cbc PRIVATE imgui imgui-glfw imgui-opengl3)
target_include_directories(cbc SYSTEM PUBLIC ${imgui_SOURCE_DIR}  ${imgui_SOURCE_DIR}/backends)

if(COUCHBASE_CXX_CLIENT_STATIC_BORINGSSL AND WIN32)
  # Ignore the `LNK4099: PDB ['crypto.pdb'|'ssl.pdb'] was not found` warnings, as we don't (atm) keep track fo the *.PDB
  # from the BoringSSL build
  set_target_properties(cbc PROPERTIES LINK_FLAGS "/ignore:4099")
endif()
