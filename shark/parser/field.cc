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
#include <shark/parser/field.h>
#include <shark/parser/pool.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace shark::parser {

    bool FieldNode::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool FieldNode::parse_type_name(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a type name, but got `%s`", t.view());
            return false;
        }
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
                report_error(ErrorType::SYNTAX, "expected a type name, but got `%s`", part.view());
                return false;
            }
            end = part.end;
            lexer.consume();
        }

        type_name = std::string_view(begin, static_cast<size_t>(end - begin));
        return true;
    }

    bool FieldNode::parse(Lexer<ExpressionLexer>& lexer) {
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
            default_value = pool->create_node<ConstValue>(file);
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

        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        tailing_comment = pool->create_node<CommentNode>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            return false;
        }
        return true;
    }

} // namespace shark::parser
