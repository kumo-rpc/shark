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

#include <shark/parser/field.h>
#include <shark/parser/pool.h>
#include <shark/parser/type_name.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace shark::parser {

    bool FieldParser::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool FieldParser::parse_type_name(Lexer<ExpressionLexer>& lexer) {
        if (!parse_type_ref(*this, lexer, type_ref)) {
            return false;
        }
        generic = std::holds_alternative<TypeName>(type_ref);
        return true;
    }

    bool FieldParser::parse(Lexer<ExpressionLexer>& lexer, End end) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a field label, but got `%s`", t.view());
            return false;
        }
        if (t == kRequiredKey) {
            label = FieldLabel::REQUIRED;
        } else if (t == kOptionalKey) {
            label = FieldLabel::OPTIONAL;
        } else if (t == kConstExprKey) {
            label = FieldLabel::CONSTEXPR;
        } else if (t == kAtomicKey) {
            label = FieldLabel::ATOMIC;
        } else {
            report_error(ErrorType::SYNTAX, "expected a field label, but got `%s`", t.view());
            return false;
        }
        start_location = {t.line, t.column};
        lexer.consume();

        if (!skip_gap(lexer) || !parse_type_name(lexer) || !skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a field name, but got `%s`", t.view());
            return false;
        }
        name = t.view();
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::At) {
            report_error(ErrorType::SYNTAX, "expected `@`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::Number) {
            report_error(ErrorType::SYNTAX, "expected a field number, but got `%s`", t.view());
            return false;
        }
        {
            auto text = t.view();
            if (text.find_first_of(".eEpP") != std::string_view::npos) {
                report_error(ErrorType::SYNTAX, "field number `%s` is not an integer", t.view());
                return false;
            }
            std::string number(text);
            char* end = nullptr;
            const bool is_hex = text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
            errno = 0;
            const unsigned long long parsed = std::strtoull(number.c_str(), &end, is_hex ? 16 : 10);
            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "field number `%s` is out of range", t.view());
                return false;
            }
            constexpr unsigned long long kMax = std::numeric_limits<uint16_t>::max();
            if (parsed == 0 || parsed > kMax) {
                report_error(ErrorType::SYNTAX, "field number `%s` must be in 1..%llu", t.view(), kMax);
                return false;
            }
            id = static_cast<uint16_t>(parsed);
        }
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type == TokenType::Equals) {
            lexer.consume();
            default_value = pool->create_node<ConstValueParser>(file);
            if (!default_value->parse(lexer)) {
                return false;
            }
            if (!skip_gap(lexer)) {
                return false;
            }
            t = lexer.look_at();
        } else if (label == FieldLabel::CONSTEXPR) {
            report_error(ErrorType::SYNTAX, "constexpr field `%s` requires a value", name);
            return false;
        }

        if (end == End::PARAMETER) {
            if (!skip_gap(lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type == TokenType::Comma || t.type == TokenType::ClosingRoundBracket) {
                report_debug(ErrorType::NONE, "field `%s` generic `%d` type `%s`", name, static_cast<int>(generic), format_type_ref(type_ref).c_str());
                return true;
            }
            report_error(ErrorType::SYNTAX, "expected `,` or `)`, but got `%s`", t.view());
            return false;
        }

        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        tailing_comment = pool->create_node<CommentParser>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            return false;
        }
        report_debug(ErrorType::NONE, "field `%s` generic `%d` type `%s`", name, static_cast<int>(generic), format_type_ref(type_ref).c_str());
        return true;
    }

} // namespace shark::parser
