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
        NAME arena_test
        MODULE memory
        SOURCES
        arena_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_memory_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)

kmcmake_cc_test(
        NAME arenastring_test
        MODULE memory
        SOURCES
        arenastring_unittest.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_memory_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)


kmcmake_cc_test(
        NAME arenaz_sampler_test
        MODULE memory
        SOURCES
        arenaz_sampler_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_memory_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)


kmcmake_cc_test(
        NAME lite_arena_test
        MODULE memory
        SOURCES
        lite_arena_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_memory_static
        ${PROJECT_NAME}::map_lite_test_util_static
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::test_common_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock_main
)