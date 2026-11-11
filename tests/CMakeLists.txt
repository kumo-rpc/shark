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

include(shark_cc_generate)

add_subdirectory(base)
add_subdirectory(compiler)


shark_schema_generate(
        TEST_FILES
        TEST_HDRS
        SCHEMAS
        test-import.shk
        test-import2.shk
        test.shk
)

kmcmake_cc_library(
        NAME test_util
        NAMESPACE shark
        SOURCES
        test-util.cc
        ${TEST_FILES}
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
)

shark_schema_generate(
        COMP_FILES
        COMP_HDRS
        SCHEMAS
        compat/json-test.shk
)

kmcmake_cc_test(
        NAME all_test
        MODULE compat
        SOURCES
        ${COMP_FILES}
        compat/byte-stream-test.cc
        compat/http-over-capnp-old-test.cc
        compat/http-over-capnp-perf-test.cc
        compat/http-over-capnp-test.cc
        compat/json-rpc-test.cc
        compat/json-test.cc
        compat/websocket-rpc-test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::testing_static
        shark::test_util_static
        shark::shark-rpc_static
        shark::shark-format_static
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
)

set(TESTS_CASE
        any-test
        #afl-testcase
        blob-test
        canonicalize-test
        capability-test
        common-test
        dynamic-test
        encoding-test
        ez-rpc-test
        #fuzz-test
        layout-test
        membrane-test
        message-test
        orphan-test
        reconnect-test
        rpc-test
        rpc-twoparty-test
        schema-loader-test
        schema-parser-test
        schema-test
        serialize-async-test
        serialize-packed-test
        serialize-text-test
)


foreach (case IN LISTS TESTS_CASE)

    kmcmake_cc_test(
            NAME ${case}
            MODULE sk
            SOURCES
            ${case}.cc
            CXXOPTS
            ${KMCMAKE_CXX_OPTIONS}
            LINKS
            shark::testing_static
            shark::test_util_static
            shark::shark-rpc_static
            shark::sharkc_static
            shark::shark-format_static
            shark::shark_static
            shark::shark_base_static
            ${KMCMAKE_DEPS_LINK}
    )

endforeach ()