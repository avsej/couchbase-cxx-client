add_library(couchbase_observability logger.cxx otel_tracer.cxx)
set_target_properties(couchbase_observability PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(
  couchbase_observability
  PUBLIC opentelemetry_logs
         opentelemetry_trace)

target_include_directories(couchbase_observability PRIVATE ${PROJECT_BINARY_DIR}/generated ${PROJECT_SOURCE_DIR})
