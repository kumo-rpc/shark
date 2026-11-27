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
#include <shark/parser/pool.h>

#include <cerrno>
#include <cstdlib>
#include <limits>

namespace shark::parser {

    bool ConstValueParser::skip(Lexer<ExpressionLexer>& lexer) {
        while (true) {
            auto t = lexer.look_at();
            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
            if (t.type != TokenType::Whitespace && t.type != TokenType::Comment) {
                return true;
            }
            lexer.consume();
        }
    }

    bool ConstValueParser::parse_number(Lexer<ExpressionLexer>& lexer, bool negative) {
        auto t = lexer.look_at();
        if (t.type != TokenType::Number) {
            report_error(ErrorType::SYNTAX, "expected a number, but got `%s`", t.view());
            return false;
        }

        auto text = t.view();
        std::string number(text);
        char* end = nullptr;
        const bool is_hex = text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
        const bool is_float = is_hex
            ? text.find_first_of(".pP") != std::string_view::npos
            : text.find_first_of(".eE") != std::string_view::npos;

        errno = 0;
        if (is_float) {
            double result = std::strtod(number.c_str(), &end);
            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid floating-point value `%s`", t.view());
                return false;
            }
            value = negative ? -result : result;
        } else {
            int64_t result = std::strtoll(number.c_str(), &end, is_hex ? 16 : 10);
            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid integer value `%s`", t.view());
                return false;
            }
            if (negative) {
                if (result < 0 || result > std::numeric_limits<int64_t>::max()) {
                    report_error(ErrorType::SYNTAX, "invalid integer value `-%s`", t.view());
                    return false;
                }
                result = -result;
            }
            value = result;
        }

        lexer.consume();
        return true;
    }

    bool ConstValueParser::parse_identifier(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
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
            auto name = lexer.look_at();
            if (name.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(name.type));
                return false;
            }
            if (name.type != TokenType::BareWord || name.begin != dot.end) {
                report_error(ErrorType::SYNTAX, "expected an identifier, but got `%s`", name.view());
                return false;
            }
            end = name.end;
            lexer.consume();
        }

        value = Identifier{std::string_view(begin, static_cast<size_t>(end - begin)), nullptr, nullptr};
        return true;
    }

    bool ConstValueParser::parse_list(Lexer<ExpressionLexer>& lexer) {
        lexer.consume();
        List list;

        if (!skip(lexer)) {
            return false;
        }
        auto t = lexer.look_at();
        if (t.type == TokenType::ClosingSquareBracket) {
            lexer.consume();
            value = std::move(list);
            return true;
        }

        while (true) {
            auto* element = pool->create_node<ConstValueParser>(file);
            if (!element->parse(lexer)) {
                return false;
            }
            list.elements.push_back(element);

            if (!skip(lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type == TokenType::Comma) {
                lexer.consume();
                if (!skip(lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type == TokenType::ClosingSquareBracket) {
                    lexer.consume();
                    value = std::move(list);
                    return true;
                }
                continue;
            }
            if (t.type == TokenType::ClosingSquareBracket) {
                lexer.consume();
                value = std::move(list);
                return true;
            }
            report_error(ErrorType::SYNTAX, "expected `,` or `]`, but got `%s`", t.view());
            return false;
        }
    }

    bool ConstValueParser::parse_map(Lexer<ExpressionLexer>& lexer) {
        lexer.consume();
        Map map;

        if (!skip(lexer)) {
            return false;
        }
        auto t = lexer.look_at();
        if (t.type == TokenType::ClosingCurlyBrace) {
            lexer.consume();
            value = std::move(map);
            return true;
        }

        while (true) {
            auto* key = pool->create_node<ConstValueParser>(file);
            if (!key->parse(lexer)) {
                return false;
            }
            if (!skip(lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type != TokenType::Equals) {
                report_error(ErrorType::SYNTAX, "expected `=`, but got `%s`", t.view());
                return false;
            }
            lexer.consume();

            auto* entry = pool->create_node<ConstValueParser>(file);
            if (!entry->parse(lexer)) {
                return false;
            }
            map.entries.emplace_back(key, entry);

            if (!skip(lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type == TokenType::Comma) {
                lexer.consume();
                if (!skip(lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type == TokenType::ClosingCurlyBrace) {
                    lexer.consume();
                    value = std::move(map);
                    return true;
                }
                continue;
            }
            if (t.type == TokenType::ClosingCurlyBrace) {
                lexer.consume();
                value = std::move(map);
                return true;
            }
            report_error(ErrorType::SYNTAX, "expected `,` or `}`, but got `%s`", t.view());
            return false;
        }
    }

    bool ConstValueParser::parse(Lexer<ExpressionLexer>& lexer) {
        if (!skip(lexer)) {
            return false;
        }

        auto t = lexer.look_at();
        start_location = {t.line, t.column};

        if (t.type == TokenType::OpeningSquareBracket) {
            return parse_list(lexer);
        }
        if (t.type == TokenType::OpeningCurlyBrace) {
            return parse_map(lexer);
        }
        if (t.type == TokenType::StringLiteral || t.type == TokenType::QuotedIdentifier) {
            value = t.view();
            lexer.consume();
            return true;
        }
        if (t.type == TokenType::Minus) {
            lexer.consume();
            if (!skip(lexer)) {
                return false;
            }
            return parse_number(lexer, true);
        }
        if (t.type == TokenType::Number) {
            return parse_number(lexer, false);
        }
        if (t.type == TokenType::BareWord && t == "true") {
            value = true;
            lexer.consume();
            return true;
        }
        if (t.type == TokenType::BareWord && t == "false") {
            value = false;
            lexer.consume();
            return true;
        }
        if (t.type == TokenType::BareWord) {
            return parse_identifier(lexer);
        }

        report_error(ErrorType::SYNTAX, "expected a value, but got `%s`", t.view());
        return false;
    }

} // namespace shark::parser
