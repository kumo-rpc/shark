#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

find_package(benchmark REQUIRED)
configure_file(${PROJECT_SOURCE_DIR}/benchmark/config.h.in ${PROJECT_SOURCE_DIR}/benchmark/config.h @ONLY)

set_target_properties(capnp_tool PROPERTIES
        CAPNP_INCLUDE_DIRECTORY "${PROJECT_SOURCE_DIR}/shark")
set_target_properties(capnpc_cpp PROPERTIES
        CAPNPC_CXX_EXECUTABLE "$<TARGET_FILE:capnpc_cpp>")

include(CapnProtoMacros)
set(CAPNPC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(CAPNPC_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
set(CAPNPC_IMPORT_DIRS "${PROJECT_SOURCE_DIR}/shark")
set(_schema_import "${CMAKE_CURRENT_BINARY_DIR}/schema-import/capnp")
file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/schema-import")
file(MAKE_DIRECTORY "${_schema_import}/compat" "${_schema_import}/compiler")
file(GLOB _root_schemas "${PROJECT_SOURCE_DIR}/shark/*.skp" "${PROJECT_SOURCE_DIR}/shark/*.skp.h")
file(GLOB _compat_schemas "${PROJECT_SOURCE_DIR}/shark/compat/*.skp" "${PROJECT_SOURCE_DIR}/shark/compat/*.skp.h")
file(GLOB _compiler_schemas "${PROJECT_SOURCE_DIR}/shark/compiler/*.skp" "${PROJECT_SOURCE_DIR}/shark/compiler/*.skp.h")
foreach(_schema ${_root_schemas} ${_compat_schemas} ${_compiler_schemas})
    get_filename_component(_name "${_schema}" NAME)
    get_filename_component(_dir "${_schema}" DIRECTORY)
    if(_dir STREQUAL "${PROJECT_SOURCE_DIR}/shark")
        set(_link "${_schema_import}/${_name}")
    elseif(_dir STREQUAL "${PROJECT_SOURCE_DIR}/shark/compat")
        set(_link "${_schema_import}/compat/${_name}")
    else()
        set(_link "${_schema_import}/compiler/${_name}")
    endif()
    file(CREATE_LINK "${_schema}" "${_link}" SYMBOLIC)
endforeach()
list(APPEND CAPNPC_IMPORT_DIRS "${CMAKE_CURRENT_BINARY_DIR}/schema-import")
capnp_generate_cpp(json_bench_sources json_bench_headers capnp/compat/json-bench.skp)
set_source_files_properties(${json_bench_sources} PROPERTIES LANGUAGE CXX)

kmcmake_cc_benchmark(
    NAME async-bench
    MODULE kj
    SOURCES kj/async-bench.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS benchmark::benchmark ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static
)
kmcmake_cc_benchmark(
    NAME table-bench
    MODULE kj
    SOURCES kj/table-bench.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS benchmark::benchmark ${PROJECT_NAME}::kj_static
)
kmcmake_cc_benchmark(
    NAME http-bench
    MODULE kj
    SOURCES kj/compat/http-bench.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS benchmark::benchmark ${PROJECT_NAME}::capnp-http_static ${PROJECT_NAME}::capnp-rpc_static ${PROJECT_NAME}::kj-http_static ${PROJECT_NAME}::kj-async_static ${PROJECT_NAME}::kj_static
)
kmcmake_cc_benchmark(
    NAME json-bench
    MODULE capnp
    SOURCES capnp/compat/json-bench.cc ${json_bench_sources}
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/gen ${CMAKE_CURRENT_BINARY_DIR}/schema-import
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS benchmark::benchmark ${PROJECT_NAME}::capnp-json_static ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
)
kmcmake_cc_benchmark(
    NAME http-over-capnp-bench
    MODULE capnp
    SOURCES capnp/compat/http-over-capnp-bench.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS benchmark::benchmark ${PROJECT_NAME}::capnp-http_static ${PROJECT_NAME}::capnp-rpc_static ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj-http_static ${PROJECT_NAME}::kj_static
)
kmcmake_cc_benchmark(
    NAME runner
    MODULE capnp
    SOURCES runner.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS benchmark::benchmark ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
)
