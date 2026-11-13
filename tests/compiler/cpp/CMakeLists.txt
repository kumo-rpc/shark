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
        NAME bootstrap_unittest
        MODULE compiler_cpp
        SOURCES
        bootstrap_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME message_size_unittest
        MODULE compiler_cpp
        SOURCES
        message_size_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME metadata_test
        MODULE compiler_cpp
        SOURCES
        metadata_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_compiler_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME move_unittest
        MODULE compiler_cpp
        SOURCES
        move_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME plugin_unittest
        MODULE compiler_cpp
        SOURCES
        plugin_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)


kmcmake_cc_test(
        NAME unittest
        MODULE compiler_cpp
        SOURCES
        unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_compiler_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_lib_static
        ${PROJECT_NAME}::sharkc_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)
