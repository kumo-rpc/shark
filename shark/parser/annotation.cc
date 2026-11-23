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

#include <shark/parser/annotation.h>

#include "pool.h"

#include <shark/parser/constants.h>

namespace shark::parser {

    bool AnnotationNode::parse_one(
        Lexer<ExpressionLexer>& lexer) {

        auto t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type == TokenType::StringLiteral || t.type == TokenType::QuotedIdentifier) {
            payload.emplace_back(t.view());
            lexer.consume();
            return true;
        }

        if (t.type != TokenType::Number) {
            report_error(ErrorType::SYNTAX, "expected an annotation value, but got `%s`", t.view());
            return false;
        }

        auto value = t.view();
        std::string number(value);
        char* end = nullptr;

        // Determine whether the number is an integer or a floating-point value.
        bool is_hex = value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X');

        bool is_float = is_hex
            ? value.find_first_of(".pP") != std::string_view::npos
            : value.find_first_of(".eE") != std::string_view::npos;

        errno = 0;

        if (is_float) {
            float result = std::strtof(number.c_str(), &end);

            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid floating-point annotation value `%s`", t.view());
                return false;
            }

            payload.emplace_back(result);
        } else {
            int64_t result = std::strtoll(number.c_str(), &end, is_hex ? 16 : 10);

            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid integer annotation value `%s`", t.view());
                return false;
            }

            payload.emplace_back(result);
        }

        lexer.consume();
        return true;
    }

    bool AnnotationNode::parse(
        Lexer<ExpressionLexer>& lexer) {

        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::BareWord || t != kAnnotationKey) {
            report_error(ErrorType::SYNTAX, "expected `annotation`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse optional classification.
        classify = { };

        if (t.type == TokenType::Dot) {
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            if (t.type != TokenType::BareWord) {
                report_error(ErrorType::SYNTAX, "expected an annotation classification, but got `%s`", t.view());
                return false;
            }

            classify = t.view();

            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();
        }

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse annotation key.
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected an annotation key, but got `%s`", t.view());
            return false;
        }

        key = t.view();

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::Equals) {
            report_error(ErrorType::SYNTAX, "expected `=`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        payload.clear();

        // Parse a single value or an array.
        if (t.type == TokenType::OpeningSquareBracket) {
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            if (t.type != TokenType::ClosingSquareBracket) {
                while (true) {
                    if (!parse_one(lexer)) {
                        return false;
                    }

                    lexer.consume_white_space(true);
                    t = lexer.look_at();

                    if (t.is_error()) {
                        report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                        return false;
                    }

                    if (t.type == TokenType::ClosingSquareBracket) {
                        break;
                    }

                    if (t.type != TokenType::Comma) {
                        report_error(ErrorType::SYNTAX, "expected `,` or `]`, but got `%s`", t.view());
                        return false;
                    }

                    lexer.consume();
                    lexer.consume_white_space(true);
                    t = lexer.look_at();

                    if (t.is_error()) {
                        report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                        return false;
                    }
                }
            }

            lexer.consume();
        } else {
            if (!parse_one(lexer)) {
                return false;
            }
        }

        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        // Parse trailing comments.
        tailing_comment = pool->create_node<CommentNode>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            return false;
        }

        return true;
    }

} // namespace shark::parser
