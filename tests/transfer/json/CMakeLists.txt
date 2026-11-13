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

kmcmake_cc_test(
        NAME default_value_objectwriter_test
        MODULE transfer_json
        SOURCES
        default_value_objectwriter_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_transfer_json_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_common_json_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME json_objectwriter_test
        MODULE transfer_json
        SOURCES
        json_objectwriter_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME json_stream_parser_test
        MODULE transfer_json
        SOURCES
        json_stream_parser_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME json_util_test
        MODULE transfer_json
        SOURCES
        json_util_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_transfer_json_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME protostream_objectsource_test
        MODULE transfer_json
        SOURCES
        protostream_objectsource_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_transfer_json_static
        ${PROJECT_NAME}::test_common_json_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME protostream_objectwriter_test
        MODULE transfer_json
        SOURCES
        protostream_objectwriter_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_transfer_json_static
        ${PROJECT_NAME}::test_common_json_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME type_resolver_util_test
        MODULE transfer_json
        SOURCES
        type_resolver_util_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_transfer_json_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::sk_lib_static
        ${PROJECT_NAME}::test_common_json_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)