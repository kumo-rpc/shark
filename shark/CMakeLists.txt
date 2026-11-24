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

# Default exported target (install(EXPORT) requires at least one).
# Teaching demos (foo / xxd / binaries) live in CMakeLists.examples.txt
# and are installed only when generating with -DKMCMAKE_GEN_EXAMPLES=ON.

kmcmake_cc_interface(
        NAMESPACE ${PROJECT_NAME}
        NAME skills
        HEADERS
        skills.h
        version.h
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        PUBLIC
)

kmcmake_cc_library(
        NAME shark-parser
        NAMESPACE shark
        SOURCES
        common/filepath.cc
        common/clog.cc
        parser/lexer.cc
        parser/parser.cc
        parser/comment.cc
        parser/edition.cc
        parser/import.cc
        parser/package.cc
        parser/annotation.cc
        parser/enum_value.cc
        parser/enum.cc
        parser/symbol.cc
        parser/file.cc
        parser/pool.cc
        parser/node.cc
        parser/const_value.cc
        parser/field.cc
        parser/struct.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)




kmcmake_cc_binary(
        NAME sharkc
        NAMESPACE shark
        SOURCES
        compiler/main.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        LINKS
        shark::shark-parser_static
        ${KMCMAKE_DEPS_LINK}
        PUBLIC
)

