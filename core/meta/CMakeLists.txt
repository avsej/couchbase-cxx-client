add_library(couchbase_meta OBJECT version.cxx)
set_target_properties(couchbase_meta PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(
  couchbase_meta
  PRIVATE project_options
          project_warnings
          snappy
asio
        llhttp::llhttp
        Microsoft.GSL::GSL
          fmt::fmt
        spdlog::spdlog)
target_include_directories(couchbase_meta PRIVATE ${PROJECT_BINARY_DIR}/generated ${PROJECT_SOURCE_DIR})

if(NOT COUCHBASE_CXX_CLIENT_POST_LINKED_OPENSSL)
  if (TARGET PkgConfig::PKG_CONFIG_OPENSSL)
    target_link_libraries(couchbase_meta PUBLIC PkgConfig::PKG_CONFIG_OPENSSL)
  else()
    target_link_libraries(couchbase_meta PUBLIC OpenSSL::SSL OpenSSL::Crypto)
  endif()
endif()
