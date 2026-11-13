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
        NAME command_line_interface_unittest
        MODULE compiler
        SOURCES
        command_line_interface_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_lib_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_compiler_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME importer_unittest
        MODULE compiler
        SOURCES
        importer_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME parser_enum_test
        MODULE compiler
        SOURCES
        parser_enum_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_lib_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME parser_unittest
        MODULE compiler
        SOURCES
        parser_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_lib_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_binary(
        NAME test_plugin
        SOURCES
        test_plugin.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_common_compiler_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock
)

add_subdirectory(cpp)
add_subdirectory(python)