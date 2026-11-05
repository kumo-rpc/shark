# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

configure_file(${PROJECT_SOURCE_DIR}/tests/config.h.in ${PROJECT_SOURCE_DIR}/tests/config.h @ONLY)

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
foreach(_schema ${_root_schemas})
    get_filename_component(_name "${_schema}" NAME)
    file(CREATE_LINK "${_schema}" "${_schema_import}/${_name}" SYMBOLIC)
endforeach()
foreach(_schema ${_compat_schemas})
    get_filename_component(_name "${_schema}" NAME)
    file(CREATE_LINK "${_schema}" "${_schema_import}/compat/${_name}" SYMBOLIC)
endforeach()
foreach(_schema ${_compiler_schemas})
    get_filename_component(_name "${_schema}" NAME)
    file(CREATE_LINK "${_schema}" "${_schema_import}/compiler/${_name}" SYMBOLIC)
endforeach()
list(APPEND CAPNPC_IMPORT_DIRS "${CMAKE_CURRENT_BINARY_DIR}/schema-import")
capnp_generate_cpp(test_capnp_sources test_capnp_headers
        capnp/test.skp
        capnp/test-import.skp
        capnp/test-import2.skp
        capnp/compat/json-test.skp)
set_source_files_properties(${test_capnp_sources} PROPERTIES LANGUAGE CXX)


kmcmake_cc_library(
    NAME kj-test
    NAMESPACE ${PROJECT_NAME}
    SOURCES kj/test.cc
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS ${PROJECT_NAME}::kj_static
)

set(kj_tests_sources
    kj/common-test.cc
    kj/memory-test.cc
    kj/array-test.cc
    kj/list-test.cc
    kj/string-test.cc
    kj/table-test.cc
    kj/map-test.cc
    kj/maybe-test.cc
    kj/exception-test.cc
    kj/debug-test.cc
    kj/io-test.cc
    kj/mutex-test.cc
    kj/time-test.cc
    kj/test-test.cc
    kj/glob-filter-test.cc
    kj/std/iostream-test.cc
)
kmcmake_cc_test(
    NAME kj-tests
    MODULE kj
    SOURCES ${kj_tests_sources}
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    DEPS ${PROJECT_NAME}::kj-test_static
    LINKS ${PROJECT_NAME}::kj-test_static ${PROJECT_NAME}::kj_static
)

if(NOT CAPNP_LITE)
    set(kj_heavy_sources
        kj/async-test.cc
        kj/async-xthread-test.cc
        kj/async-coroutine-test.cc
        kj/async-coroutine-alloc-test.cc
        kj/async-unix-test.cc
        kj/async-unix-xthread-test.cc
        kj/async-win32-test.cc
        kj/async-win32-xthread-test.cc
        kj/async-io-test.cc
        kj/async-queue-test.cc
        kj/sticky-flag-test.cc
        kj/refcount-test.cc
        kj/string-tree-test.cc
        kj/encoding-test.cc
        kj/arena-test.cc
        kj/units-test.cc
        kj/tuple-test.cc
        kj/one-of-test.cc
        kj/function-test.cc
        kj/filesystem-test.cc
        kj/filesystem-disk-test.cc
        kj/thread-test.cc
        kj/parse/common-test.cc
        kj/parse/char-test.cc
        kj/compat/url-test.cc
        kj/compat/http-test.cc
        kj/compat/gzip-test.cc
        kj/compat/tls-test.cc
    )
    set(_kj_heavy_links
        ${PROJECT_NAME}::kj-http_static
        ${PROJECT_NAME}::kj-async_static
        ${PROJECT_NAME}::kj-test_static
        ${PROJECT_NAME}::kj_static
    )
    if(WITH_OPENSSL)
        list(APPEND _kj_heavy_links ${PROJECT_NAME}::kj-tls_static OpenSSL::SSL OpenSSL::Crypto)
    endif()
    if(WITH_ZLIB)
        list(APPEND _kj_heavy_links ${PROJECT_NAME}::kj-gzip_static)
    endif()
    kmcmake_cc_test(
        NAME kj-heavy-tests
        MODULE kj
        SOURCES ${kj_heavy_sources}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        DEPS ${PROJECT_NAME}::kj-test_static
        LINKS ${_kj_heavy_links}
    )
endif()

set(capnp_tests_sources
    capnp/common-test.cc
    capnp/blob-test.cc
    capnp/endian-test.cc
    capnp/endian-fallback-test.cc
    capnp/layout-test.cc
    capnp/any-test.cc
    capnp/message-test.cc
    capnp/encoding-test.cc
    capnp/orphan-test.cc
    capnp/serialize-test.cc
    capnp/serialize-packed-test.cc
    capnp/canonicalize-test.cc
    capnp/fuzz-test.cc
    capnp/test-util.cc
)
set(_capnp_test_links ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj-test_static ${PROJECT_NAME}::kj_static)
if(NOT CAPNP_LITE)
    list(APPEND _capnp_test_links
        ${PROJECT_NAME}::capnp-json_static
        ${PROJECT_NAME}::capnp-rpc_static
        ${PROJECT_NAME}::capnp-websocket_static
        ${PROJECT_NAME}::capnpc_static
        ${PROJECT_NAME}::kj-http_static
        ${PROJECT_NAME}::kj-async_static
    )
endif()
kmcmake_cc_test(
    NAME capnp-tests
    MODULE capnp
    SOURCES ${capnp_tests_sources} ${test_capnp_sources} ${test_capnp_headers}
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/gen ${CMAKE_CURRENT_BINARY_DIR}/schema-import
    CXXOPTS ${KMCMAKE_CXX_OPTIONS}
    LINKS ${_capnp_test_links}
)

if(NOT CAPNP_LITE)
    kmcmake_cc_test(
        NAME capnp-heavy-tests
        MODULE capnp
        SOURCES
            capnp/endian-reverse-test.cc
            capnp/capability-test.cc
            capnp/membrane-test.cc
            capnp/schema-test.cc
            capnp/schema-loader-test.cc
            capnp/schema-parser-test.cc
            capnp/dynamic-test.cc
            capnp/stringify-test.cc
            capnp/serialize-async-test.cc
            capnp/serialize-text-test.cc
            capnp/rpc-test.cc
            capnp/rpc-twoparty-test.cc
            capnp/compiler/lexer-test.cc
            capnp/compiler/type-id-test.cc
            capnp/test-util.cc
            capnp/compat/json-test.cc
            capnp/compat/websocket-rpc-test.cc
            ${test_capnp_sources}
            ${test_capnp_headers}
        INCLUDES ${CMAKE_CURRENT_BINARY_DIR}/gen ${CMAKE_CURRENT_BINARY_DIR}/schema-import
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${_capnp_test_links}
    )
    kmcmake_cc_test(
        NAME capnp-evolution-tests
        MODULE capnp
        SOURCES capnp/compiler/evolution-test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${PROJECT_NAME}::capnpc_static ${PROJECT_NAME}::capnp_static ${PROJECT_NAME}::kj_static
    )
endif()
