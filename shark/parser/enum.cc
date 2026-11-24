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

#include <shark/parser/constants.h>
#include <shark/parser/enum.h>
#include <shark/parser/file.h>
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>
#include <shark/parser/enum_value.h>

namespace shark::parser {

    bool EnumNode::bind_enum() {
        bind = true;
        for (EnumValueNode* value : values) {
            value->bind = true;
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
