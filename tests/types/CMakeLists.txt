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
        NAME any_test
        MODULE types
        SOURCES
        any_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)
kmcmake_cc_test(
        NAME map_field_test
        MODULE types
        SOURCES
        map_field_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME map_test
        MODULE types
        SOURCES
        map_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_lib_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME narrow_int_reflect_test
        MODULE types
        SOURCES
        narrow_int_reflect_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME narrow_int_test
        MODULE types
        SOURCES
        narrow_int_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME repeated_field_reflection_test
        MODULE types
        SOURCES
        repeated_field_reflection_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)
kmcmake_cc_test(
        NAME repeated_field_test
        MODULE types
        SOURCES
        repeated_field_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME well_known_types_test
        MODULE types
        SOURCES
        well_known_types_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)
