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

    class FileNode : public SymbolNode {
    public:

        /// 1. setting
        /// 2. checkfile exists
        /// 3. load file data.
        /// 4. initialize lexer
        bool load(std::string_view absolute_path, std::string_view path);

        bool parse();

        bool bind_file();

        SymbolNode* find_type(std::string_view name) const;
        FieldNode* find_constexpr(std::string_view name) const;
        SymbolNode* concrete_type(SymbolNode* symbol) const;


        std::string                 file_data;
        std::string                 path;
        std::string                 absolute_path;
        std::string                 package;

        Edition                     *edition{nullptr};
        std::vector<PackageNode*>    packages;
        std::vector<ImportNode*>     imports;

        std::vector<CommentNode*>    leading_comment;
        CommentNode                 *tailing_comment{nullptr};
        mutable std::vector<AnnotationNode*> annotations;

        std::vector<EnumNode*>       enums;
        std::vector<StructNode*>     structs;
        std::vector<ServiceNode*>    services;
        std::vector<ServletNode*>    servlets;
        std::vector<FunctionNode*>   functions;
        std::vector<FieldNode*>      constexprs;
        std::vector<FileNode*>       dependencies;
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
        bool assign_enum(EnumNode* node, std::string_view scope, std::vector<std::string>& seen);
        bool assign_struct(StructNode* node, std::string_view scope, std::vector<std::string>& seen);
        bool assign_function(FunctionNode* node, std::string_view scope, std::vector<std::string>& seen);
        bool assign_servlet(ServletNode* node, std::string_view scope, std::vector<std::string>& seen);
        bool bind_constexpr_round1();
        bool bind_constexpr_round2();
        bool bind_aliases();
        bool bind_alias_struct(StructNode* node, bool& progress);
        bool bind_alias_struct_list(bool& progress);
        bool bind_constexpr_field(FieldNode* field, bool final_round);
        bool bind_constexpr_struct(StructNode* node, bool final_round);
        bool bind_enums();
        bool bind_enums_in(StructNode* node);
        bool bind_structs();
        bool bind_struct_pass(StructNode* node, bool& progress, bool& pending);
        bool bind_functions();
        bool bind_servlets();
        SymbolNode* find_local(std::string_view name) const;
        SymbolNode* find_local_struct(const StructNode* node, std::string_view name) const;

        bool parse_header();
        bool parse_leading(std::vector<CommentNode*>& comments, std::vector<AnnotationNode*>& annotations);
    protected:
        friend class RegistryPool;
        FileNode(const FileNode* f, RegistryPool * pool) :SymbolNode(NodeType::FILE, nullptr, pool) {}

    };
}  // namespace shark::parser
