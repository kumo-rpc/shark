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


set(UNIT_MEM
        unittest_arena.shark
)
kmcmake_cpp_test_sk(
        NAME test_mem_sk
        OUTDIR ${PROJECT_SOURCE_DIR}
        PROTOS ${UNIT_MEM}
)

kmcmake_cc_library(
        NAME sk_memory
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${test_mem_sk_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        ${PROJECT_NAME}::test_sk_static
        GTest::gmock
)

