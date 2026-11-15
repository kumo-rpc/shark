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
        unittest_lazy_dependencies.shark
        unittest_lazy_dependencies_custom_option.shark
        unittest_lazy_dependencies_enum.shark
)
shark_schema_generate(
        sk_lib_lazy_obj_SRCS
        sk_lib_lazy_obj_HDRS
        OUTDIR ${PROJECT_SOURCE_DIR}
        PREFIX ${PROJECT_SOURCE_DIR}
        SCHEMAS ${UNIT_SHARK}
)

kmcmake_cc_library(
        NAME sk_lib_lazy
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_lib_lazy_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)


set(UNIT_SHARK_LIB
        unittest_custom_options.shark
)
shark_schema_generate(
        sk_lib_obj_SRCS
        sk_lib_obj_HDRS
        OUTDIR ${PROJECT_SOURCE_DIR}
        PREFIX ${PROJECT_SOURCE_DIR}
        SCHEMAS ${UNIT_SHARK_LIB}
)

kmcmake_cc_library(
        NAME sk_lib
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${sk_lib_obj_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)

#############################################
shark_schema_generate(
        drop_unknown_fields_sk_SRCS
        drop_unknown_fields_sk_HDRS
        OUTDIR ${PROJECT_SOURCE_DIR}
        PREFIX ${PROJECT_SOURCE_DIR}
        SCHEMAS unittest_drop_unknown_fields.shark
)

kmcmake_cc_library(
        NAME sk_lib_drop_unknown_fields
        NAMESPACE ${PROJECT_NAME}
        SOURCES
        ${drop_unknown_fields_sk_SRCS}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${PROJECT_NAME}::shark-lite_static
        GTest::gmock
)
