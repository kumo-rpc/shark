// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <shark/compiler/generator.h>
#include <shark/compiler/cpp/constexpr.h>
#include <shark/compiler/cpp/enum.h>
#include <shark/compiler/cpp/function.h>
#include <shark/compiler/cpp/servlet.h>
#include <shark/compiler/cpp/struct.h>

namespace shark::compiler::cpp {

    class FileGenerator {
    public:
        explicit FileGenerator(const shark::SchemaFile* file) : _file(file) {

        }
        ~FileGenerator() = default;

        void generate_header(io::Printer &printer);
        void generate_source(io::Printer &printer);
    private:
        void generate_file_comment(io::Printer &printer);
        void generate_namespace_start(io::Printer &printer);
        void generate_namespace_end(io::Printer &printer);

        void generate_header_include(io::Printer &printer);
        void generate_source_include(io::Printer &printer);

        void initialize();
    private:
        const shark::SchemaFile* _file{nullptr};
        std::vector<ConstexprGenerator> _constexpr_generators;
        std::vector<EnumGenerator> _enum_generators;
        std::vector<StructGenerator> _struct_generators;
        std::vector<ServletGenerator> _servlet_generators;
        std::vector<FunctionGenerator> _function_generators;
    };
}  // namespace shark::compiler::cpp
