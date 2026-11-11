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

set(COMPILER_TEST_LINKS
        shark::testing_static
        shark::sharkc_static
        shark::shark-format_static
        shark::shark_static
        shark::shark_base_static
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME lexer_test
        MODULE compiler
        SOURCES lexer-test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${COMPILER_TEST_LINKS}
)

kmcmake_cc_test(
        NAME type_id_test
        MODULE compiler
        SOURCES type-id-test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${COMPILER_TEST_LINKS}
)

kmcmake_cc_test(
        NAME evolution_test
        MODULE compiler
        SOURCES evolution-test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        LINKS ${COMPILER_TEST_LINKS}
        COMMAND compiler_evolution_test --seed 1
)

add_test(
        NAME compiler_shark_test
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/shark-test.sh
)
set_tests_properties(compiler_shark_test PROPERTIES
        ENVIRONMENT "SHARK=$<TARGET_FILE:shark::shark>"
)
