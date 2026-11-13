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
        NAME descriptor_database_test
        MODULE lib
        SOURCES
        descriptor_database_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME descriptor_test
        MODULE lib
        SOURCES
        descriptor_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::sk_lib_static
        ${PROJECT_NAME}::sk_lib_lazy_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

#############################################

kmcmake_cc_test(
        NAME drop_unknown_fields_test
        MODULE lib
        SOURCES
        drop_unknown_fields_test.cc
        ${unittest_drop_unknown_fields_sk_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_lib_drop_unknown_fields_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

#############################################
kmcmake_cc_test(
        NAME dynamic_message_test
        MODULE lib
        SOURCES
        dynamic_message_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

#############################################
kmcmake_cc_test(
        NAME extension_set_test
        MODULE lib
        SOURCES
        extension_set_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::map_lite_test_util_static
        ${PROJECT_NAME}::sk_format_wire_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)


#############################################
kmcmake_cc_test(
        NAME lite_test
        MODULE lib
        SOURCES
        lite_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::map_lite_test_util_static
        ${PROJECT_NAME}::sk_format_wire_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)


#############################################
kmcmake_cc_test(
        NAME message_test
        MODULE lib
        SOURCES
        message_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)


#############################################
kmcmake_cc_test(
        NAME reflection_ops_test
        MODULE lib
        SOURCES
        reflection_ops_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME unknown_field_set_unittest
        MODULE lib
        SOURCES
        unknown_field_set_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)