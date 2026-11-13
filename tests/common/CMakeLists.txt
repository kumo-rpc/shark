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

kmcmake_cc_library(
        NAME test_common_lib
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        lib/reflection_tester.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)


kmcmake_cc_library(
        NAME test_common
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        arena_test_util.cc
        file.cc
        googletest.cc
        test_util.cc
        test_util_lite.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)

kmcmake_cc_library(
        NAME map_lite_test_util
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        types/map_lite_test_util.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_lib_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)

kmcmake_cc_library(
        NAME test_common_compiler
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        compiler/annotation_test_util.cc
        compiler/mock_code_generator.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_lib_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)


kmcmake_cc_library(
        NAME test_common_json
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        json/type_info_test_helper.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_common_lib_static
        ${PROJECT_NAME}::sk_types_static
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)