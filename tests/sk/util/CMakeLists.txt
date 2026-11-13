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
        NAME sk_util_obj
        OUTDIR ${PROJECT_SOURCE_DIR}
        PROTOS
        message_differencer_unittest.shark
)

kmcmake_cc_library(
        NAME sk_util
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_util_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock
)
