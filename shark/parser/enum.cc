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

#include <shark/parser/const_value.h>
#include <shark/parser/constants.h>
#include <shark/parser/enum.h>
#include <shark/parser/enum_value.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>

#include <cstdint>
#include <limits>
#include <variant>

namespace shark::parser {

    namespace {

        bool is_integer_builtin(const StructNode* node) {
            if (node == nullptr || !node->builtin) {
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

        bool value_in_underlying(EnumNode::UnderlyingType type, int64_t number) {
            switch (type) {
            case EnumNode::UnderlyingType::INT8:
                return number >= std::numeric_limits<int8_t>::min() && number <= std::numeric_limits<int8_t>::max();
            case EnumNode::UnderlyingType::UINT8:
                return number >= 0 && number <= std::numeric_limits<uint8_t>::max();
            case EnumNode::UnderlyingType::INT16:
                return number >= std::numeric_limits<int16_t>::min() && number <= std::numeric_limits<int16_t>::max();
            case EnumNode::UnderlyingType::UINT16:
                return number >= 0 && number <= std::numeric_limits<uint16_t>::max();
            case EnumNode::UnderlyingType::NONE:
            case EnumNode::UnderlyingType::INT32:
                return number >= std::numeric_limits<int32_t>::min() && number <= std::numeric_limits<int32_t>::max();
            case EnumNode::UnderlyingType::UINT32:
                return number >= 0 && number <= std::numeric_limits<uint32_t>::max();
            }
            return false;
        }

    }  // namespace

    bool EnumNode::bind_enum() {
        for (EnumValueNode* node : values) {
            if (!node->constexpr_ref.empty()) {
                FieldNode* field = file->find_constexpr(node->constexpr_ref);
                if (field == nullptr) {
                    report_error(ErrorType::SEMANTIC, "enum value `%s` constexpr `%s` was not found", node->name, node->constexpr_ref);
                    return false;
                }
                if (!field->bind || !is_integer_builtin(field->type)) {
                    report_error(ErrorType::SEMANTIC, "enum value `%s` constexpr `%s` is not an integer", node->name, node->constexpr_ref);
                    return false;
                }
                if (field->default_value == nullptr || !std::holds_alternative<int64_t>(field->default_value->value)) {
                    report_error(ErrorType::SEMANTIC, "enum value `%s` constexpr `%s` is not an integer constant", node->name, node->constexpr_ref);
                    return false;
                }
                node->value = std::get<int64_t>(field->default_value->value);
            }
            if (!value_in_underlying(underlying, node->value)) {
                report_error(ErrorType::SEMANTIC, "enum value `%s` is out of range", node->name);
                return false;
            }
            node->bind = true;
            if (!node->emit_to_pool()) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", node->full_name);
                return false;
            }
        }
        bind = true;
        if (!emit_to_pool()) {
            report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", full_name);
            return false;
        }
        return true;
    }

    bool EnumNode::parse(
        Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse enum keyword.
        if (t.type != TokenType::BareWord || t != "enum") {
            report_error(ErrorType::SYNTAX, "expected `enum`, but got `%s`", t.view());
            return false;
        }

        start_location = {t.line, t.column};

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse optional class keyword.
        is_class = false;
        underlying = UnderlyingType::NONE;

        if (t.type == TokenType::BareWord && t == "class") {
            is_class = true;
            underlying = UnderlyingType::INT32;

            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
        }

        // Parse enum name.
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected an enum name, but got `%s`", t.view());
            return false;
        }

        name = t.view();

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse optional underlying type.
        if (t.type == TokenType::Colon) {
            if (!is_class) {
                report_error(
                    ErrorType::SYNTAX,
                    "underlying type is only allowed for `enum class`, but got `enum %s`",
                    name);
                return false;
            }

            if (!parse_underlying(lexer)) {
                return false;
            }

            t = lexer.look_at();
        }

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse opening brace.
        if (t.type != TokenType::OpeningCurlyBrace) {
            report_error(ErrorType::SYNTAX, "expected `{`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        // Parse enum values.
        while (true) {
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            if (t.type == TokenType::ClosingCurlyBrace) {
                break;
            }

            if (t.is_end()) {
                report_error(ErrorType::SYNTAX, "expected `}`, but reached end of input");
                return false;
            }

            auto value = pool->create_symbol<EnumValueNode>(file);
            if (!value->parse(lexer)) {
                return false;
            }

            values.emplace_back(value);
        }

        // Consume closing brace.
        lexer.consume();

        // Parse trailing comment before checking the optional semicolon.
        tailing_comment = pool->create_node<CommentNode>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "enum `%s` class `%d` values `%zu`", name, static_cast<int>(is_class), values.size());
            return false;
        }

        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse optional semicolon.
        if (t.type == TokenType::Semicolon) {
            lexer.consume();

            if (!tailing_comment->parse_tailing(lexer)) {
                report_debug(ErrorType::NONE, "enum `%s` class `%d` values `%zu`", name, static_cast<int>(is_class), values.size());
                return false;
            }
        }

        report_debug(ErrorType::NONE, "enum `%s` class `%d` values `%zu`", name, static_cast<int>(is_class), values.size());
        return true;
    }

    bool EnumNode::parse_underlying(
        Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();

        // Consume ':'.
        if (t.type != TokenType::Colon) {
            report_error(ErrorType::SYNTAX, "expected `:`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::BareWord) {
            report_error(
                ErrorType::SYNTAX,
                "expected an underlying type (`int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`), but got `%s`",
                t.view());
            return false;
        }

        if (t == "int8") {
            underlying = UnderlyingType::INT8;
        } else if (t == "uint8") {
            underlying = UnderlyingType::UINT8;
        } else if (t == "int16") {
            underlying = UnderlyingType::INT16;
        } else if (t == "uint16") {
            underlying = UnderlyingType::UINT16;
        } else if (t == "int32") {
            underlying = UnderlyingType::INT32;
        } else if (t == "uint32") {
            underlying = UnderlyingType::UINT32;
        } else {
            report_error(ErrorType::SYNTAX, "unsupported enum underlying type `%s`", t.view());
            return false;
        }

        lexer.consume();
        lexer.consume_white_space(true);

        return true;
    }

} // namespace shark::parser
