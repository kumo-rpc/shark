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


set(UNIT_SHARK
        test_bad_identifiers.shark
        test_large_enum_value.shark
        unittest_embed_optimize_for.shark
        unittest_optimize_for.shark
        unittest_no_generic_services.shark
        unittest_enormous_descriptor.shark
)
kmcmake_cpp_test_sk(
        NAME sk_compiler_obj
        OUTDIR ${PROJECT_SOURCE_DIR}
        PROTOS ${UNIT_SHARK}
)

kmcmake_cc_library(
        NAME sk_compiler
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_compiler_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::test_sk_static
        ${PROJECT_NAME}::shark_static
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)
