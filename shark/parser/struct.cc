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
#include <shark/parser/builtin.h>
#include <shark/parser/enum.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>
#include <shark/parser/type_name.h>

namespace shark::parser {

namespace {

    void mark_parameters(const std::vector<TypeName>& parameters, TypeName& type) {
        if (type.arguments.empty()) {
            for (const TypeName& parameter : parameters) {
                if (parameter.name == type.name) {
                    type.is_parameter = true;
                    return;
                }
            }
            return;
        }
        for (TypeName& argument : type.arguments) {
            mark_parameters(parameters, argument);
        }
    }

    bool type_uses(const TypeName& type, std::string_view parameter) {
        if (type.arguments.empty() && type.name == parameter) {
            return true;
        }
        for (const TypeName& argument : type.arguments) {
            if (type_uses(argument, parameter)) {
                return true;
            }
        }
        return false;
    }

    bool uses_parameter(const TypeName& type) {
        if (type.is_parameter) {
            return true;
        }
        for (const TypeName& argument : type.arguments) {
            if (uses_parameter(argument)) {
                return true;
            }
        }
        return false;
    }

}  // namespace

    StructParser::BindStep StructParser::bind_struct() {
        if (bind) {
            return BindStep::DONE;
        }
        for (AnnotationParser* annotation : annotations) {
            if (annotation->classify != "builtin" || annotation->key != "phy_type") {
                continue;
            }
            const auto* text = annotation->object == nullptr ? nullptr : std::get_if<std::string_view>(&annotation->object->value);
            if (text == nullptr || text->size() < 2) {
                report_error(ErrorType::SEMANTIC, "struct `%s` physical type is missing", name);
                return BindStep::FAIL;
            }
            std::string_view phy_name = text->substr(1, text->size() - 2);
            std::optional<PhysicalType> phy = physical_type(phy_name);
            if (!phy.has_value()) {
                report_error(ErrorType::SEMANTIC, "struct `%s` physical type `%s` is unknown", name, phy_name);
                return BindStep::FAIL;
            }
            phy_type = *phy;
            builtin = true;
        }
        for (std::size_t i = 0; i < fields.size(); ++i) {
            for (std::size_t j = i + 1; j < fields.size(); ++j) {
                if (fields[i]->id != fields[j]->id) {
                    continue;
                }
                report_error(ErrorType::SEMANTIC, "field `%s` number `%u` duplicates field `%s`", fields[j]->name, static_cast<unsigned>(fields[j]->id), fields[i]->name);
                return BindStep::FAIL;
            }
        }
        bool waiting = false;
        for (FieldParser* field : fields) {
            if (field->bind) {
                continue;
            }
            mark_parameters(declared_name.arguments, field->type_ref);
            if (field->type_ref.is_parameter) {
                field->generic = true;
                field->bind = true;
                if (!field->emit_to_pool()) {
                    return BindStep::FAIL;
                }
                continue;
            }
            if (uses_parameter(field->type_ref)) {
                field->generic = true;
            }
            SymbolNode* symbol = file->find_type(field->type_ref);
            if (symbol != nullptr) {
                symbol = file->concrete_type(symbol);
            }
            if (symbol == nullptr) {
                report_error(ErrorType::SEMANTIC, "field `%s` type `%s` was not found", name, type_ref_name(field->type_ref));
                return BindStep::FAIL;
            }
            if (symbol->node_type == SymbolType::ALIAS) {
                if (!symbol->bind) {
                    waiting = true;
                    continue;
                }
                report_error(ErrorType::SEMANTIC, "field `%s` type `%s` alias is cyclic", name, type_ref_name(field->type_ref));
                return BindStep::FAIL;
            }
            if (symbol->node_type == SymbolType::STRUCT) {
                auto* type = static_cast<StructParser*>(symbol);
                if (type->builtin || type->bind) {
                    field->type = type;
                    field->bind = true;
                    if (!field->emit_to_pool()) {
                        return BindStep::FAIL;
                    }
                    continue;
                }
                waiting = true;
                continue;
            }
            if (symbol->node_type == SymbolType::ENUM) {
                auto* enum_type = static_cast<EnumParser*>(symbol);
                if (!enum_type->bind) {
                    report_error(ErrorType::SEMANTIC, "field `%s` type `%s` was not bound", name, type_ref_name(field->type_ref));
                    return BindStep::FAIL;
                }
                field->enum_type = enum_type;
                field->bind = true;
                if (!field->emit_to_pool()) {
                    return BindStep::FAIL;
                }
                continue;
            }
            report_error(ErrorType::SEMANTIC, "field `%s` type `%s` must be a struct, union, enum, or builtin", name, type_ref_name(field->type_ref));
            return BindStep::FAIL;
        }
        if (waiting) {
            return BindStep::WAIT;
        }
        for (const TypeName& parameter : declared_name.arguments) {
            bool used = false;
            for (const FieldParser* field : fields) {
                if (type_uses(field->type_ref, parameter.name)) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                report_debug(ErrorType::SEMANTIC, "generic parameter `%s` of `%s` is unused", parameter.name, name);
            }
        }
        bind = true;
        if (!emit_to_pool()) {
            return BindStep::FAIL;
        }
        return BindStep::DONE;
    }

namespace {

    bool is_field_label(const Token& t) {
        return t.type == TokenType::BareWord
            && (t == kRequiredKey || t == kOptionalKey || t == kConstExprKey || t == kAtomicKey);
    }

} // namespace

    bool StructParser::parse_field(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentParser*>& comments,
        std::vector<AnnotationParser*>& annotations) {
        auto* field = pool->create_symbol<FieldParser>(file);
        field->container = this;
        field->leading_comment = std::move(comments);
        field->annotations = std::move(annotations);
        if (!field->parse(lexer)) {
            return false;
        }
        fields.push_back(field);
        return true;
    }

    bool StructParser::parse_alias(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentParser*>& comments,
        std::vector<AnnotationParser*>& annotations) {
        auto* alias = pool->create_symbol<AliasParser>(file);
        alias->container = this;
        alias->leading_comment = std::move(comments);
        alias->annotations = std::move(annotations);
        if (!alias->parse(lexer)) {
            return false;
        }
        aliases.push_back(alias);
        return true;
    }

    bool StructParser::parse_nested_enum(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentParser*>& comments,
        std::vector<AnnotationParser*>& annotations) {
        auto* enum_node = pool->create_symbol<EnumParser>(file);
        enum_node->container = this;
        enum_node->leading_comment = std::move(comments);
        enum_node->annotations = std::move(annotations);
        if (!enum_node->parse(lexer)) {
            return false;
        }
        nested_enums.push_back(enum_node);
        return true;
    }

    bool StructParser::parse_nested_struct(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentParser*>& comments,
        std::vector<AnnotationParser*>& annotations) {
        auto* nested = pool->create_symbol<StructParser>(file);
        nested->container = this;
        nested->leading_comment = std::move(comments);
        nested->annotations = std::move(annotations);
        if (!nested->parse(lexer)) {
            return false;
        }
        nested_types.push_back(nested);
        return true;
    }

    bool StructParser::parse_nested_union(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentParser*>& comments,
        std::vector<AnnotationParser*>& annotations) {
        auto* nested = pool->create_symbol<StructParser>(file);
        nested->container = this;
        nested->leading_comment = std::move(comments);
        nested->annotations = std::move(annotations);
        if (!nested->parse(lexer)) {
            return false;
        }
        nested_types.push_back(nested);
        return true;
    }

    bool StructParser::parse(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type != TokenType::BareWord || (t != kStructKey && t != kUnionKey)) {
            report_error(ErrorType::SYNTAX, "expected `struct` or `union`, but got `%s`", t.view());
            return false;
        }

        is_union = t == kUnionKey;
        start_location = {t.line, t.column};
        lexer.consume();
        lexer.consume_white_space(true);

        if (!parse_type_ref(*this, lexer, declared_name)) {
            return false;
        }
        name = declared_name.name;
        generic = !declared_name.arguments.empty();

        t = lexer.look_at();
        if (t.type != TokenType::OpeningCurlyBrace) {
            report_error(ErrorType::SYNTAX, "expected `{`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        while (true) {
            std::vector<CommentParser*> comments;
            std::vector<AnnotationParser*> annotations;
            while (true) {
                auto* comment = pool->create_node<CommentParser>(file);
                if (!comment->parse_leading(lexer)) {
                    return false;
                }
                if (!comment->empty()) {
                    comments.push_back(comment);
                }

                t = lexer.look_at();
                if (t.is_error()) {
                    report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                    return false;
                }
                if (t.type == TokenType::BareWord && t == kAnnotationKey) {
                    auto* annotation = pool->create_node<AnnotationParser>(file);
                    if (!annotation->parse(lexer)) {
                        return false;
                    }
                    if (annotation->file_level) {
                        annotation->report_warn(
                            ErrorType::SYNTAX,
                            "file-level annotation belongs in the file scope, not inside an object definition");
                        file->annotations.push_back(annotation);
                    } else {
                        annotations.push_back(annotation);
                    }
                    continue;
                }
                break;
            }

            if (t.type == TokenType::ClosingCurlyBrace) {
                break;
            }
            if (is_field_label(t)) {
                if (!parse_field(lexer, comments, annotations)) {
                    return false;
                }
                continue;
            }
            if (t.type == TokenType::BareWord && (t == kUsingKey || t == kTypeDefKey)) {
                if (!parse_alias(lexer, comments, annotations)) {
                    return false;
                }
                continue;
            }
            if (t.type == TokenType::BareWord && t == kEnumKey) {
                if (!parse_nested_enum(lexer, comments, annotations)) {
                    return false;
                }
                continue;
            }
            if (t.type == TokenType::BareWord && t == kStructKey) {
                if (!parse_nested_struct(lexer, comments, annotations)) {
                    return false;
                }
                continue;
            }
            if (t.type == TokenType::BareWord && t == kUnionKey) {
                if (!parse_nested_union(lexer, comments, annotations)) {
                    return false;
                }
                continue;
            }

            report_error(ErrorType::SYNTAX, "expected a field, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        tailing_comment = pool->create_node<CommentParser>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "%s `%s` fields `%zu`", is_union ? "union" : "struct", name, fields.size());
            return false;
        }

        t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type == TokenType::Semicolon) {
            lexer.consume();
            if (!tailing_comment->parse_tailing(lexer)) {
                report_debug(ErrorType::NONE, "%s `%s` fields `%zu`", is_union ? "union" : "struct", name, fields.size());
                return false;
            }
        }

        report_debug(ErrorType::NONE, "%s `%s` fields `%zu`", is_union ? "union" : "struct", name, fields.size());
        return true;
    }

} // namespace shark::parser
