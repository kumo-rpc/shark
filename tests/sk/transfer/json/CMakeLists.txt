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
        NAME sk_transfer_json_obj
        OUTDIR ${PROJECT_SOURCE_DIR}
        PROTOS
        anys.shark
        books.shark
        default_value.shark
        default_value_test.shark
        field_mask.shark
        maps.shark
        oneofs.shark
        struct.shark
        timestamp_duration.shark
        wrappers.shark
        json_format.shark
        json_format_proto3.shark
)

kmcmake_cc_library(
        NAME sk_transfer_json
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_transfer_json_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_format_wire_sk_static
        ${PROJECT_NAME}::shark-lite_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock
)
