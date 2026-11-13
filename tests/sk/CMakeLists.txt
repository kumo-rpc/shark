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


add_subdirectory(format)

kmcmake_cpp_test_sk(
        NAME sk_obj
        OUTDIR ${PROJECT_SOURCE_DIR}
        PROTOS
        unittest.shark
        unittest_import.shark
        unittest_import_lite.shark
        unittest_import_public.shark
        unittest_import_public_lite.shark
        unittest_lite.shark
        unittest_lite_imports_nonlite.shark
        unittest_mset.shark
)

kmcmake_cc_library(
        NAME test_sk
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::sk_format_wire_sk_static
        ${PROJECT_NAME}::shark-lite_static
        ${PROJECT_NAME}::shark_static
        GTest::gmock
)

add_subdirectory(compiler)
add_subdirectory(types)
add_subdirectory(memory)
add_subdirectory(lib)
add_subdirectory(util)
add_subdirectory(transfer)
