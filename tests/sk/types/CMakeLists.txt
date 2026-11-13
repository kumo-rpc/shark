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

kmcmake_cpp_test_sk(
        NAME sk_types_obj
        OUTDIR ${PROJECT_SOURCE_DIR}
        PROTOS
        map_lite_unittest.shark
        any_test.shark
        map_proto2_unittest.shark
        map_unittest.shark
        narrow_int.shark
        unittest_empty.shark
        unittest_well_known_types.shark
)

kmcmake_cc_library(
        NAME sk_types
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_types_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_format_wire_sk_static
        ${PROJECT_NAME}::shark-lite_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock
)
