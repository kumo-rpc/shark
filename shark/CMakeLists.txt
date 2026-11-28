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
        parser/import_graph.cc
        parser/package.cc
        parser/annotation.cc
        parser/builtin.cc
        parser/enum_value.cc
        parser/enum.cc
        parser/primitive.cc
        parser/symbol.cc
        parser/file.cc
        parser/pool.cc
        parser/node.cc
        parser/const_value.cc
        parser/field.cc
        parser/type_name.cc
        parser/struct.cc
        parser/function.cc
        parser/alias.cc
        parser/servlet.cc
        parser/export.cc
        schema/schema_pool.cc
        schema/schema_pool_api.cc
        schema/constrain.cc
        schema/uri.cc
        schema/annotation.cc
        io/printer.cc
        io/zero_copy_stream.cc
        io/memory_zero_copy_stream.cc
        io/file_zero_copy_stream.cc
        compiler/generator.cc
        compiler/cpp/generator.cc
        compiler/cpp/enum.cc
        compiler/cpp/struct.cc
        compiler/cpp/field.cc
        compiler/cpp/function.cc
        compiler/cpp/servlet.cc
        compiler/cpp/file.cc
        compiler/cpp/constexpr.cc
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

