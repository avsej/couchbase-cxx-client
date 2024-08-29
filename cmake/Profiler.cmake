option(COUCHBASE_CXX_CLIENT_PROFILER "Build with profiler (gperftools) support" FALSE)

if(COUCHBASE_CXX_CLIENT_PROFILER)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBPROFILER REQUIRED libprofiler)
    if(NOT LIBPROFILER_FOUND)
        message(FATAL_ERROR "COUCHBASE_CXX_CLIENT_PROFILER requested, but libprofiler (gperftools) is not found")
    endif()
endif()

function(enable_profiler target)
    if(COUCHBASE_CXX_CLIENT_PROFILER)
        target_link_libraries(${target} PRIVATE ${LIBPROFILER_LIBRARIES})
    endif()
endfunction()

option(COUCHBASE_CXX_CLIENT_TRACY "Build with profiler (Tracy) support" TRUE)
set(COUCHBASE_CXX_CLIENT_TRACY_ROOT "" CACHE PATH "Path to Tracy installation (optional)")

if(COUCHBASE_CXX_CLIENT_TRACY)
    set(tracy_hints)

    if(COUCHBASE_CXX_CLIENT_TRACY_ROOT)
        list(APPEND tracy_hints "${COUCHBASE_CXX_CLIENT_TRACY_ROOT}")
    endif()

    if(EXISTS "$ENV{HOME}/.local")
        list(APPEND tracy_hints "$ENV{HOME}/.local")
    endif()

    find_package(Tracy CONFIG QUIET HINTS ${tracy_hints})

    if(Tracy_FOUND)
        message(STATUS "Tracy profiler found - profiling enabled")
        set(COUCHBASE_CXX_CLIENT_TRACY_ENABLED ON CACHE INTERNAL "")
    else()
        message(STATUS "Tracy profiler not found - profiling disabled")
        set(COUCHBASE_CXX_CLIENT_TRACY_ENABLED OFF CACHE INTERNAL "")
    endif()
else()
    set(COUCHBASE_CXX_CLIENT_TRACY_ENABLED OFF CACHE INTERNAL "")
endif()

function(enable_tracy target)
    if(NOT COUCHBASE_CXX_CLIENT_TRACY_ENABLED)
        return()
    endif()

    target_link_libraries(${target} PRIVATE Tracy::TracyClient)
    target_compile_definitions(${target} PRIVATE TRACY_ENABLE)

    if(NOT MSVC)
        target_compile_options(${target} PRIVATE -g -fno-omit-frame-pointer -rdynamic)
    endif()
endfunction()
