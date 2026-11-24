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

#include <shark/parser/alias.h>
#include <shark/parser/constants.h>
#include <shark/parser/enum.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/servlet.h>
#include <shark/parser/struct.h>
#include <shark/parser/type_name.h>

#include <turbo/log/logging.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

namespace shark::parser {

    bool FileNode::load(std::string_view absolute_path_in, std::string_view path_in) {
        KLOG(INFO)<<1<<222;
        if (absolute_path_in.empty() || path_in.empty()) {
            report_error(ErrorType::IO, "path and absolute_path must both be non-empty");
            return false;
        }

        absolute_path.assign(absolute_path_in.data(), absolute_path_in.size());
        path.assign(path_in.data(), path_in.size());

        std::error_code ec;
        if (!std::filesystem::is_regular_file(absolute_path, ec)) {
            report_error(ErrorType::IO, "file `%s` does not exist", absolute_path);
            return false;
        }

        std::ifstream in(absolute_path, std::ios::in | std::ios::binary);
        if (!in) {
            report_error(ErrorType::IO, "failed to open `%s`", absolute_path);
            return false;
        }

        const std::istreambuf_iterator<char> begin(in);
        const std::istreambuf_iterator<char> end;
        std::string data(begin, end);
        if (in.bad()) {
            report_error(ErrorType::IO, "failed to read `%s`", absolute_path);
            return false;
        }

        lexer.reset();
        file_data = std::move(data);
        lexer = std::make_unique<Lexer<ExpressionLexer>>(file_data);
        return true;
    }

    bool FileNode::bind_file() {
        if (!assign_names()) {
            return false;
        }
        if (!bind_aliases()) {
            return false;
        }
        if (!bind_constexpr_round1()) {
            return false;
        }
        if (!bind_enums()) {
            return false;
        }
        if (!bind_aliases()) {
            return false;
        }
        if (!bind_constexpr_round2()) {
            return false;
        }
        if (!bind_structs()) {
            return false;
        }
        if (!bind_functions()) {
            return false;
        }
        if (!bind_servlets()) {
            return false;
        }
        bind = true;
        return true;
    }

    bool FileNode::take_package() {
        for (PackageNode* node : packages) {
            if (!node->classify.empty()) {
                continue;
            }
            std::string_view raw = node->package;
            if (raw.size() < 2) {
                report_error(ErrorType::SEMANTIC, "package name is empty");
                return false;
            }
            package.assign(raw.data() + 1, raw.size() - 2);
            return true;
        }
        report_error(ErrorType::MISSING, "file `%s` has no package", path);
        return false;
    }

    bool FileNode::assign_symbol(SymbolNode* symbol, std::string_view scope, std::vector<std::string>& seen) {
        if (scope.empty()) {
            symbol->full_name_without_package.assign(symbol->name.data(), symbol->name.size());
        } else {
            symbol->full_name_without_package.clear();
            symbol->full_name_without_package.append(scope);
            symbol->full_name_without_package.push_back('.');
            symbol->full_name_without_package.append(symbol->name);
        }
        symbol->full_name = package;
        symbol->full_name.push_back('.');
        symbol->full_name += symbol->full_name_without_package;

        for (const std::string& existing : seen) {
            if (existing == symbol->full_name) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", symbol->full_name);
                return false;
            }
        }
        seen.push_back(symbol->full_name);
        return true;
    }

    bool FileNode::assign_enum(EnumNode* node, std::string_view scope, std::vector<std::string>& seen) {
        if (!assign_symbol(node, scope, seen)) {
            return false;
        }
        std::string_view value_scope = node->is_class ? std::string_view(node->full_name_without_package) : scope;
        for (EnumValueNode* value : node->values) {
            if (!assign_symbol(value, value_scope, seen)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::assign_struct(StructNode* node, std::string_view scope, std::vector<std::string>& seen) {
        if (!assign_symbol(node, scope, seen)) {
            return false;
        }
        std::string_view nested = node->full_name_without_package;
        for (EnumNode* child : node->nested_enums) {
            if (!assign_enum(child, nested, seen)) {
                return false;
            }
        }
        for (StructNode* child : node->nested_types) {
            if (!assign_struct(child, nested, seen)) {
                return false;
            }
        }
        for (FieldNode* field : node->fields) {
            if (!assign_symbol(field, nested, seen)) {
                return false;
            }
        }
        for (AliasNode* alias : node->aliases) {
            if (!assign_symbol(alias, nested, seen)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::assign_function(FunctionNode* node, std::string_view scope, std::vector<std::string>& seen) {
        if (!assign_symbol(node, scope, seen)) {
            return false;
        }
        std::string_view nested = node->full_name_without_package;
        for (FieldNode* param : node->params) {
            if (!assign_symbol(param, nested, seen)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::assign_servlet(ServletNode* node, std::string_view scope, std::vector<std::string>& seen) {
        if (!assign_symbol(node, scope, seen)) {
            return false;
        }
        std::string_view nested = node->full_name_without_package;
        for (FunctionNode* method : node->method) {
            if (!assign_function(method, nested, seen)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::assign_names() {
        if (!take_package()) {
            return false;
        }
        std::vector<std::string> seen;
        for (EnumNode* node : enums) {
            if (!assign_enum(node, {}, seen)) {
                return false;
            }
        }
        for (StructNode* node : structs) {
            if (!assign_struct(node, {}, seen)) {
                return false;
            }
        }
        for (ServletNode* node : servlets) {
            if (!assign_servlet(node, {}, seen)) {
                return false;
            }
        }
        for (FunctionNode* node : functions) {
            if (!assign_function(node, {}, seen)) {
                return false;
            }
        }
        for (FieldNode* node : constexprs) {
            if (!assign_symbol(node, {}, seen)) {
                return false;
            }
        }
        return true;
    }

    namespace {

        bool same_name(const SymbolNode* symbol, std::string_view name) {
            return symbol->full_name == name || symbol->full_name_without_package == name;
        }

        FieldNode* find_constexpr_in(const StructNode* node, std::string_view name) {
            for (FieldNode* field : node->fields) {
                if (field->label == FieldLabel::CONSTEXPR && same_name(field, name)) {
                    return field;
                }
            }
            for (StructNode* child : node->nested_types) {
                if (FieldNode* found = find_constexpr_in(child, name)) {
                    return found;
                }
            }
            return nullptr;
        }

        bool is_scalar_builtin(const StructNode* node) {
            if (node == nullptr || !node->builtin) {
                return false;
            }
            switch (node->phy_type) {
            case PhysicalType::BOOL:
            case PhysicalType::INT8:
            case PhysicalType::UINT8:
            case PhysicalType::INT16:
            case PhysicalType::UINT16:
            case PhysicalType::INT32:
            case PhysicalType::UINT32:
            case PhysicalType::INT64:
            case PhysicalType::UINT64:
            case PhysicalType::INT128:
            case PhysicalType::UINT128:
            case PhysicalType::FLOAT32:
            case PhysicalType::FLOAT64:
            case PhysicalType::STRING:
                return true;
            default:
                return false;
            }
        }

        bool is_integer_builtin(const StructNode* node) {
            if (!node->builtin) {
                return false;
            }
            switch (node->phy_type) {
            case PhysicalType::INT8:
            case PhysicalType::UINT8:
            case PhysicalType::INT16:
            case PhysicalType::UINT16:
            case PhysicalType::INT32:
            case PhysicalType::UINT32:
            case PhysicalType::INT64:
            case PhysicalType::UINT64:
                return true;
            default:
                return false;
            }
        }

    }  // namespace

    SymbolNode* FileNode::find_local_struct(const StructNode* node, std::string_view name) const {
        if (same_name(node, name)) {
            return const_cast<StructNode*>(node);
        }
        for (EnumNode* child : node->nested_enums) {
            if (same_name(child, name)) {
                return child;
            }
        }
        for (StructNode* child : node->nested_types) {
            if (SymbolNode* found = find_local_struct(child, name)) {
                return found;
            }
        }
        for (AliasNode* alias : node->aliases) {
            if (same_name(alias, name)) {
                return alias;
            }
        }
        return nullptr;
    }

    SymbolNode* FileNode::find_local(std::string_view name) const {
        for (EnumNode* node : enums) {
            if (same_name(node, name)) {
                return node;
            }
        }
        for (StructNode* node : structs) {
            if (SymbolNode* found = find_local_struct(node, name)) {
                return found;
            }
        }
        return nullptr;
    }

    FieldNode* FileNode::find_constexpr(std::string_view name) const {
        for (FieldNode* field : constexprs) {
            if (same_name(field, name)) {
                return field;
            }
        }
        for (const StructNode* node : structs) {
            if (FieldNode* found = find_constexpr_in(node, name)) {
                return found;
            }
        }
        return nullptr;
    }

    SymbolNode* FileNode::find_type(std::string_view name) const {
        SymbolNode* symbol = pool->get_symbol(name);
        if (symbol != nullptr) {
            return symbol;
        }
        return find_local(name);
    }

    bool FileNode::bind_alias_struct(StructNode* node, bool& progress) {
        for (StructNode* child : node->nested_types) {
            if (!bind_alias_struct(child, progress)) {
                return false;
            }
        }
        for (AliasNode* alias : node->aliases) {
            if (alias->bind) {
                continue;
            }
            if (!alias->bind_alias()) {
                return false;
            }
            progress = true;
        }
        return true;
    }

    bool FileNode::bind_aliases() {
        bool progress = false;
        for (StructNode* node : structs) {
            if (!bind_alias_struct(node, progress)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::bind_alias_struct_list(bool& progress) {
        for (StructNode* node : structs) {
            if (!bind_alias_struct(node, progress)) {
                return false;
            }
        }
        return true;
    }

    SymbolNode* FileNode::concrete_type(SymbolNode* symbol) const {
        SymbolNode* current = symbol;
        while (current != nullptr && current->node_type == NodeType::ALIAS) {
            auto* alias = static_cast<AliasNode*>(current);
            if (!alias->bind || alias->target == nullptr) {
                return current;
            }
            current = const_cast<SymbolNode*>(alias->target);
            if (current == symbol) {
                return symbol;
            }
        }
        return current;
    }

    bool FileNode::bind_constexpr_field(FieldNode* field, bool final_round) {
        if (field->bind) {
            return true;
        }
        SymbolNode* symbol = find_type(type_ref_name(field->type_ref));
        if (symbol != nullptr) {
            symbol = concrete_type(symbol);
        }
        if (symbol == nullptr) {
            report_error(ErrorType::SEMANTIC, "constexpr `%s` type `%s` was not found", field->name, type_ref_name(field->type_ref));
            return false;
        }
        if (symbol->node_type == NodeType::ALIAS) {
            report_error(ErrorType::SEMANTIC, "constexpr `%s` type `%s` was not bound", field->name, type_ref_name(field->type_ref));
            return false;
        }
        if (final_round) {
            if (symbol->node_type == NodeType::ENUM) {
                auto* enum_type = static_cast<EnumNode*>(symbol);
                if (enum_type->bind) {
                    field->enum_type = enum_type;
                    field->bind = true;
                    if (!field->emit_to_pool()) {
                        report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", field->full_name);
                        return false;
                    }
                    return true;
                }
            }
            report_error(ErrorType::SEMANTIC, "constexpr `%s` type `%s` was not bound", field->name, type_ref_name(field->type_ref));
            return false;
        }
        if (symbol->node_type == NodeType::STRUCT) {
            auto* type = static_cast<StructNode*>(symbol);
            field->type = type;
            field->bind = true;
            if (!field->emit_to_pool()) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", field->full_name);
                return false;
            }
            if (!is_scalar_builtin(type)) {
                field->static_const = true;
                report_warn(ErrorType::SEMANTIC, "constexpr `%s` type `%s` is downgraded to static const and is not a compile-time constant", field->name, type_ref_name(field->type_ref));
            }
            return true;
        }
        if (symbol->node_type == NodeType::ENUM) {
            auto* enum_type = static_cast<EnumNode*>(symbol);
            if (!enum_type->bind) {
                return true;
            }
            field->enum_type = enum_type;
            field->bind = true;
            if (!field->emit_to_pool()) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", field->full_name);
                return false;
            }
            return true;
        }
        report_error(ErrorType::SEMANTIC, "constexpr `%s` type `%s` was not bound", field->name, type_ref_name(field->type_ref));
        return false;
    }

    bool FileNode::bind_constexpr_struct(StructNode* node, bool final_round) {
        for (FieldNode* field : node->fields) {
            if (field->label != FieldLabel::CONSTEXPR) {
                continue;
            }
            if (!bind_constexpr_field(field, final_round)) {
                return false;
            }
        }
        for (StructNode* child : node->nested_types) {
            if (!bind_constexpr_struct(child, final_round)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::bind_constexpr_round1() {
        for (FieldNode* field : constexprs) {
            if (!bind_constexpr_field(field, false)) {
                return false;
            }
        }
        for (StructNode* node : structs) {
            if (!bind_constexpr_struct(node, false)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::bind_constexpr_round2() {
        for (FieldNode* field : constexprs) {
            if (!bind_constexpr_field(field, true)) {
                return false;
            }
        }
        for (StructNode* node : structs) {
            if (!bind_constexpr_struct(node, true)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::bind_enums_in(StructNode* node) {
        for (EnumNode* child : node->nested_enums) {
            if (!child->bind_enum()) {
                return false;
            }
        }
        for (StructNode* child : node->nested_types) {
            if (!bind_enums_in(child)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::bind_enums() {
        for (EnumNode* node : enums) {
            if (!node->bind_enum()) {
                return false;
            }
        }
        for (StructNode* node : structs) {
            if (!bind_enums_in(node)) {
                return false;
            }
        }
        return true;
    }

    bool FileNode::bind_struct_pass(StructNode* node, bool& progress, bool& pending) {
        for (StructNode* child : node->nested_types) {
            if (!bind_struct_pass(child, progress, pending)) {
                return false;
            }
        }
        if (node->bind) {
            return true;
        }
        StructNode::BindStep step = node->bind_struct();
        if (step == StructNode::BindStep::FAIL) {
            return false;
        }
        if (step == StructNode::BindStep::DONE) {
            progress = true;
            return true;
        }
        pending = true;
        return true;
    }

    bool FileNode::bind_structs() {
        while (true) {
            bool progress = false;
            bool pending = false;
            for (StructNode* node : structs) {
                if (!bind_struct_pass(node, progress, pending)) {
                    return false;
                }
            }
            if (!bind_alias_struct_list(progress)) {
                return false;
            }
            if (!pending) {
                return true;
            }
            if (!progress) {
                report_error(ErrorType::SEMANTIC, "struct or union value dependency is cyclic in `%s`", path);
                return false;
            }
        }
    }

    bool FileNode::bind_functions() {
        for (FunctionNode* node : functions) {
            if (!node->bind_function()) {
                return false;
            }
        }
        for (ServletNode* servlet : servlets) {
            for (FunctionNode* method : servlet->method) {
                if (!method->bind_function()) {
                    return false;
                }
            }
        }
        return true;
    }

    bool FileNode::bind_servlets() {
        while (true) {
            bool progress = false;
            bool pending = false;
            for (ServletNode* node : servlets) {
                if (node->bind) {
                    continue;
                }
                ServletNode::BindStep step = node->bind_servlet();
                if (step == ServletNode::BindStep::FAIL) {
                    return false;
                }
                if (step == ServletNode::BindStep::DONE) {
                    progress = true;
                    continue;
                }
                pending = true;
            }
            if (!pending) {
                return true;
            }
            if (!progress) {
                report_error(ErrorType::SEMANTIC, "servlet base dependency is cyclic in `%s`", path);
                return false;
            }
        }
    }

    /// The file must start with edition. Package is a top-level element.
    bool FileNode::parse_header() {
        if (!lexer) {
            report_error(ErrorType::INTERNAL, "lexer is null");
            return false;
        }

        lexer->consume_white_space(true);
        edition = pool->create_node<Edition>(this);
        auto* edition_leading = pool->create_node<CommentNode>(this);
        if (!edition_leading->parse_leading(*lexer)) {
            return false;
        }
        if (!edition_leading->empty()) {
            edition->leading_comment = edition_leading;
        }
        return edition->parse(*lexer);
    }

    bool FileNode::parse_leading(std::vector<CommentNode*>& comments, std::vector<AnnotationNode*>& annotations) {
        while (true) {
            auto* comment = pool->create_node<CommentNode>(this);
            if (!comment->parse_leading(*lexer)) {
                return false;
            }
            if (!comment->empty()) {
                comments.push_back(comment);
            }

            auto t = lexer->look_at();
            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
            if (t.type == TokenType::EndOfStream) {
                return true;
            }
            if (t.type == TokenType::BareWord && t == "annotation") {
                auto* annotation = pool->create_node<AnnotationNode>(this);
                if (!annotation->parse(*lexer)) {
                    return false;
                }
                if (annotation->file_level) {
                    this->annotations.push_back(annotation);
                } else {
                    annotations.push_back(annotation);
                }
                continue;
            }
            return true;
        }
    }

    bool FileNode::parse() {
        if (!parse_header()) {
            return false;
        }

        while (true) {
            lexer->consume_white_space(true);
            std::vector<CommentNode*> comments;
            std::vector<AnnotationNode*> annotations;
            if (!parse_leading(comments, annotations)) {
                return false;
            }

            auto t = lexer->look_at();
            if (t.type == TokenType::EndOfStream) {
                return true;
            }
            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
            if (t.type == TokenType::BareWord && t == "package") {
                auto* package = pool->create_node<PackageNode>(this);
                if (!comments.empty()) {
                    package->leading_comment = comments.front();
                }
                if (!package->parse(*lexer)) {
                    return false;
                }
                packages.push_back(package);
                continue;
            }
            if (t.type == TokenType::BareWord && t == "enum") {
                auto* enum_node = pool->create_symbol<EnumNode>(this);
                enum_node->leading_comment = std::move(comments);
                enum_node->annotations = std::move(annotations);
                if (!enum_node->parse(*lexer)) {
                    return false;
                }
                enums.push_back(enum_node);
                continue;
            }
            if (t.type == TokenType::BareWord && (t == kStructKey || t == kUnionKey)) {
                auto* struct_node = pool->create_symbol<StructNode>(this);
                struct_node->leading_comment = std::move(comments);
                struct_node->annotations = std::move(annotations);
                if (!struct_node->parse(*lexer)) {
                    return false;
                }
                structs.push_back(struct_node);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kConstExprKey) {
                auto* constant = pool->create_symbol<FieldNode>(this);
                constant->leading_comment = std::move(comments);
                constant->annotations = std::move(annotations);
                if (!constant->parse(*lexer)) {
                    return false;
                }
                constexprs.push_back(constant);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kFunctionKey) {
                auto* function = pool->create_symbol<FunctionNode>(this);
                function->leading_comment = std::move(comments);
                function->annotations = std::move(annotations);
                if (!function->parse(*lexer)) {
                    return false;
                }
                functions.push_back(function);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kServletKey) {
                auto* servlet = pool->create_symbol<ServletNode>(this);
                servlet->leading_comment = std::move(comments);
                servlet->annotations = std::move(annotations);
                if (!servlet->parse(*lexer)) {
                    return false;
                }
                servlets.push_back(servlet);
                continue;
            }
            if (t.type == TokenType::BareWord && t == kImportKey) {
                auto* import_node = pool->create_node<ImportNode>(this);
                if (!comments.empty()) {
                    import_node->leading_comment = comments.front();
                }
                import_node->annotations = std::move(annotations);
                if (!import_node->parse(*lexer)) {
                    return false;
                }
                imports.push_back(import_node);
                continue;
            }

            report_error(ErrorType::SYNTAX, "expected `package`, `enum`, `struct`, `union`, `servlet`, `function`, `constexpr` or `import`, but got `%s`", t.view());
            return false;
        }
    }
}  // namespace shark::parser

