add_subdirectory(utils)

integration_test(connect)
integration_test(management)
integration_test(crud)
integration_test(query)
integration_test(diagnostics)
integration_test(binary_operations)
integration_test(durability)
integration_test(arithmetic)
integration_test(collections)
integration_test(subdoc)
integration_test(analytics)
integration_test(read_replica)
integration_test(tracer)
integration_test(meter)
integration_test(transcoders)
integration_test(range_scan)
integration_test(search)

add_executable(test_integration
        test_integration_connect.cxx
        test_integration_management.cxx
        test_integration_crud.cxx
        test_integration_query.cxx
        test_integration_diagnostics.cxx
        test_integration_binary_operations.cxx
        test_integration_durability.cxx
        test_integration_arithmetic.cxx
        test_integration_collections.cxx
        test_integration_subdoc.cxx
        test_integration_analytics.cxx
        test_integration_read_replica.cxx
        test_integration_tracer.cxx
        test_integration_meter.cxx
        test_integration_transcoders.cxx
        test_integration_range_scan.cxx
        test_integration_search.cxx)
target_include_directories(test_integration PRIVATE ${PROJECT_BINARY_DIR}/generated)
target_link_libraries(
        test_integration
        project_options
        project_warnings
        Catch2::Catch2WithMain
        Threads::Threads
        snappy
        couchbase_cxx_client
        test_utils)
catch_discover_tests(
        test_integration
        PROPERTIES
        SKIP_REGULAR_EXPRESSION
        "SKIP"
        LABELS
        "integration")

unit_test(connection_string)
unit_test(utils)
unit_test(json_transcoder)
unit_test(json_streaming_lexer)
unit_test(jsonsl)
unit_test(config_profiles)
unit_test(options)
target_link_libraries(test_unit_jsonsl jsonsl)

integration_benchmark(get)

transaction_test(context)
transaction_test(simple)
transaction_test(simple_async)
transaction_test(public_blocking_api)
transaction_test(public_async_api)

unit_test(transaction_logging)
unit_test(transaction_utils)
unit_test(waitable_op_list)

integration_test(examples)
transaction_test(examples)
