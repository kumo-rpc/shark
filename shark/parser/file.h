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


#include <shark/parser/symbol.h>
#include <shark/parser/edition.h>
#include <shark/parser/package.h>
#include <shark/parser/import.h>
#include <shark/parser/annotation.h>
#include <shark/parser/node.h>
#include <shark/parser/lexer.h>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace shark::parser {

    class AliasParser;

    class FileParser : public SymbolNode {
    public:

        /// 1. setting
        /// 2. checkfile exists
        /// 3. load file data.
        /// 4. initialize lexer
        bool load(std::string_view absolute_path, std::string_view path);

        bool parse();

        bool bind_file();

        SymbolNode* find_type(std::string_view name) const;
        SymbolNode* find_type(const TypeName& type) const;
        FieldParser* find_constexpr(std::string_view name) const;
        SymbolNode* concrete_type(SymbolNode* symbol) const;


        std::string                 file_data;
        std::string                 path;
        std::string                 absolute_path;
        std::string                 package;

        EditionParser                     *edition{nullptr};
        std::vector<PackageParser*>    packages;
        std::vector<ImportParser*>     imports;

        std::vector<CommentParser*>    leading_comment;
        CommentParser                 *tailing_comment{nullptr};
        mutable std::vector<AnnotationParser*> annotations;

        std::vector<EnumParser*>       enums;
        std::vector<StructParser*>     structs;
        std::vector<PrimitiveParser*>  primitives;
        std::vector<ServiceParser*>    services;
        std::vector<ServletParser*>    servlets;
        std::vector<FunctionParser*>   functions;
        std::vector<FieldParser*>      constexprs;
        std::vector<AliasParser*>      aliases;
        std::vector<FileParser*>       dependencies;
        std::unique_ptr<Lexer<ExpressionLexer>>    lexer;


        [[nodiscard]]  std::string_view get_absolute_path() const override {
            return absolute_path;
        }
        [[nodiscard]]  std::string_view get_path() const override{
            return path;
        }

    private:
        bool assign_names();
        bool take_package();
        bool assign_symbol(SymbolNode* symbol, std::string_view scope, std::vector<std::string>& seen);
        bool assign_enum(EnumParser* node, std::string_view scope, std::vector<std::string>& seen);
        bool assign_struct(StructParser* node, std::string_view scope, std::vector<std::string>& seen);
        bool assign_function(FunctionParser* node, std::string_view scope, std::vector<std::string>& seen);
        bool assign_servlet(ServletParser* node, std::string_view scope, std::vector<std::string>& seen);
        bool bind_constexpr_round1();
        bool bind_constexpr_round2();
        bool bind_aliases();
        bool bind_alias_struct(StructParser* node, bool& progress, bool& pending);
        bool bind_alias_struct_list(bool& progress, bool& pending);
        bool bind_constexpr_field(FieldParser* field, bool final_round);
        bool bind_constexpr_struct(StructParser* node, bool final_round);
        bool bind_enums();
        bool bind_enums_in(StructParser* node);
        bool bind_structs();
        bool bind_struct_pass(StructParser* node, bool& progress, bool& pending);
        bool bind_functions();
        bool bind_servlets();
        bool bind_annotations();
        SymbolNode* find_local(std::string_view name) const;
        SymbolNode* find_local_struct(const StructParser* node, std::string_view name) const;

        bool parse_header();
        bool parse_leading(std::vector<CommentParser*>& comments, std::vector<AnnotationParser*>& annotations);
    protected:
        friend class RegistryPool;
        FileParser(const FileParser* f, RegistryPool * pool) :SymbolNode(SymbolType::FILE, nullptr, pool) {}

    };
}  // namespace shark::parser
