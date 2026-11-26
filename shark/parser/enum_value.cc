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
#include <shark/parser/enum_value.h>

#include "file.h"
#include "pool.h"

#include <shark/parser/enum.h>
#include <shark/parser/struct.h>

namespace shark::parser {

    bool EnumValueParser::parse(
        Lexer<ExpressionLexer>& lexer) {
        return parse_leading(lexer) && parse_value(lexer);
    }

    bool EnumValueParser::parse_leading(
        Lexer<ExpressionLexer>& lexer) {
        while (true) {
            // Parse a leading comment block.
            auto comment = pool->create_node<CommentParser>(file);
            if (!comment->parse_leading(lexer)) {
                return false;
            }

            if (!comment->empty()) {
                leading_comment.emplace_back(comment);
            }

            auto t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            // Parse annotations preceding the enum value.
            if (t.type == TokenType::BareWord && t == "annotation") {
                auto annotation = pool->create_node<AnnotationParser>(file);
                if (!annotation->parse(lexer)) {
                    return false;
                }

                if (annotation->file_level) {
                    annotation->report_warn(
                        ErrorType::SYNTAX,
                        "file-level annotation belongs in the file scope, not inside an object definition");
                    file->annotations.push_back(annotation);
                } else {
                    annotations.emplace_back(annotation);
                }
                continue;
            }

            // The next token must be the enum value name.
            if (t.type != TokenType::BareWord) {
                report_error(ErrorType::SYNTAX, "expected an enum value name, but got `%s`", t.view());
                return false;
            }

            return true;
        }
    }

    bool EnumValueParser::parse_value(
        Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse enum value name.
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected an enum value name, but got `%s`", t.view());
            return false;
        }

        name = t.view();
        start_location = {t.line, t.column};

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse '='.
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

        // Parse optional sign.
        bool negative = false;
        if (t.type == TokenType::Minus) {
            negative = true;
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
        }

        if (negative) {
            if (t.type != TokenType::Number) {
                report_error(ErrorType::SYNTAX, "expected an integer value, but got `%s`", t.view());
                return false;
            }
        } else if (t.type == TokenType::BareWord) {
            const char* begin = t.begin;
            const char* end = t.end;
            lexer.consume();

            while (true) {
                auto dot = lexer.look_at();
                if (dot.is_error()) {
                    report_error(ErrorType::SYNTAX, "%s", get_error_token_description(dot.type));
                    return false;
                }
                if (dot.type != TokenType::Dot || dot.begin != end) {
                    break;
                }
                lexer.consume();
                auto part = lexer.look_at();
                if (part.is_error()) {
                    report_error(ErrorType::SYNTAX, "%s", get_error_token_description(part.type));
                    return false;
                }
                if (part.type != TokenType::BareWord || part.begin != dot.end) {
                    report_error(ErrorType::SYNTAX, "expected an identifier, but got `%s`", part.view());
                    return false;
                }
                end = part.end;
                lexer.consume();
            }

            constexpr_ref = std::string_view(begin, static_cast<size_t>(end - begin));
        } else if (t.type != TokenType::Number) {
            report_error(ErrorType::SYNTAX, "enum value must be an integer or a constexpr, but got `%s`", t.view());
            return false;
        }

        if (constexpr_ref.empty()) {
            std::string number(t.view());
            char* end = nullptr;

            errno = 0;
            const auto parsed = std::strtoll(number.c_str(), &end, 10);

            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid integer value `%s`", number);
                return false;
            }

            value = negative ? -parsed : parsed;
            lexer.consume();
        }
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse ';'.
        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        // Parse trailing comment.
        tailing_comment = pool->create_node<CommentParser>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "enum value `%s` = `%lld`", name, static_cast<long long>(value));
            return false;
        }

        report_debug(ErrorType::NONE, "enum value `%s` = `%lld`", name, static_cast<long long>(value));
        return true;
    }


} // namespace shark::parser
