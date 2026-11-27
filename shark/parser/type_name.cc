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

#include <shark/parser/type_name.h>
#include <shark/parser/lexer.h>
#include <shark/parser/node.h>

#include <string>

namespace shark::parser {

    namespace {

        bool skip_gap(Node& node, Lexer<ExpressionLexer>& lexer) {
            while (true) {
                auto t = lexer.look_at();
                if (t.is_error()) {
                    node.report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                    return false;
                }
                if (t.type != TokenType::Whitespace && t.type != TokenType::Comment) {
                    return true;
                }
                lexer.consume();
            }
        }

        bool is_word(const Token& token, std::string_view word) {
            return token.type == TokenType::BareWord && token.view() == word;
        }

        void append_constraint(std::string& out, const TypeConstraint& constraint);

        void append_type_name(std::string& out, const TypeName& type) {
            out.append(type.name.data(), type.name.size());
            if (!type.arguments.empty()) {
                out.push_back('<');
                for (std::size_t i = 0; i < type.arguments.size(); ++i) {
                    if (i != 0) {
                        out.append(", ");
                    }
                    append_type_name(out, type.arguments[i]);
                }
                out.push_back('>');
            }
            if (type.constraint.has_value()) {
                out.push_back('[');
                append_constraint(out, *type.constraint);
                out.push_back(']');
            }
        }

        void append_constraint(std::string& out, const TypeConstraint& constraint) {
            switch (constraint.op) {
            case TypeConstraint::Op::CALL:
                out.append(constraint.function.name.data(), constraint.function.name.size());
                if (!constraint.function.arguments.empty()) {
                    out.push_back('<');
                    for (std::size_t i = 0; i < constraint.function.arguments.size(); ++i) {
                        if (i != 0) {
                            out.append(", ");
                        }
                        append_type_name(out, constraint.function.arguments[i]);
                    }
                    out.push_back('>');
                }
                break;
            case TypeConstraint::Op::NOT:
                out.append("not ");
                append_constraint(out, constraint.operands[0]);
                break;
            case TypeConstraint::Op::AND:
                append_constraint(out, constraint.operands[0]);
                out.append(" and ");
                append_constraint(out, constraint.operands[1]);
                break;
            case TypeConstraint::Op::OR:
                append_constraint(out, constraint.operands[0]);
                out.append(" or ");
                append_constraint(out, constraint.operands[1]);
                break;
            }
        }

        bool parse_constraint(Node& node, Lexer<ExpressionLexer>& lexer, TypeConstraint& out);

        bool parse_constraint_primary(Node& node, Lexer<ExpressionLexer>& lexer, TypeConstraint& out) {
            if (!skip_gap(node, lexer)) {
                return false;
            }
            auto t = lexer.look_at();
            if (t.type == TokenType::OpeningRoundBracket) {
                lexer.consume();
                if (!parse_constraint(node, lexer, out)) {
                    return false;
                }
                if (!skip_gap(node, lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type != TokenType::ClosingRoundBracket) {
                    node.report_error(ErrorType::SYNTAX, "expected `)`, but got `%s`", t.view());
                    return false;
                }
                lexer.consume();
                return true;
            }
            if (t.type != TokenType::BareWord || is_word(t, "and") || is_word(t, "or") || is_word(t, "not")) {
                node.report_error(ErrorType::SYNTAX, "expected a constraint, but got `%s`", t.view());
                return false;
            }
            out.op = TypeConstraint::Op::CALL;
            out.function.name = t.view();
            lexer.consume();
            if (!skip_gap(node, lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type != TokenType::Less) {
                return true;
            }
            lexer.consume();
            if (!skip_gap(node, lexer)) {
                return false;
            }
            while (true) {
                t = lexer.look_at();
                if (t.type == TokenType::Greater) {
                    break;
                }
                TypeName arg;
                if (!parse_type_ref(node, lexer, arg)) {
                    return false;
                }
                out.function.arguments.push_back(std::move(arg));
                if (!skip_gap(node, lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type == TokenType::Comma) {
                    lexer.consume();
                    if (!skip_gap(node, lexer)) {
                        return false;
                    }
                    continue;
                }
                if (t.type == TokenType::Greater) {
                    break;
                }
                node.report_error(ErrorType::SYNTAX, "expected `,` or `>` in constraint `%s`, but got `%s`", out.function.name, t.view());
                return false;
            }
            if (out.function.arguments.empty()) {
                node.report_error(ErrorType::SYNTAX, "constraint `%s` requires type arguments", out.function.name);
                return false;
            }
            lexer.consume();
            return true;
        }

        bool parse_constraint_not(Node& node, Lexer<ExpressionLexer>& lexer, TypeConstraint& out) {
            if (!skip_gap(node, lexer)) {
                return false;
            }
            auto t = lexer.look_at();
            if (!is_word(t, "not")) {
                return parse_constraint_primary(node, lexer, out);
            }
            lexer.consume();
            TypeConstraint operand;
            if (!parse_constraint_not(node, lexer, operand)) {
                return false;
            }
            out.op = TypeConstraint::Op::NOT;
            out.operands.push_back(std::move(operand));
            return true;
        }

        bool parse_constraint_and(Node& node, Lexer<ExpressionLexer>& lexer, TypeConstraint& out) {
            if (!parse_constraint_not(node, lexer, out)) {
                return false;
            }
            while (true) {
                if (!skip_gap(node, lexer)) {
                    return false;
                }
                auto t = lexer.look_at();
                if (!is_word(t, "and")) {
                    return true;
                }
                lexer.consume();
                TypeConstraint right;
                if (!parse_constraint_not(node, lexer, right)) {
                    return false;
                }
                TypeConstraint combined;
                combined.op = TypeConstraint::Op::AND;
                combined.operands.push_back(std::move(out));
                combined.operands.push_back(std::move(right));
                out = std::move(combined);
            }
        }

        bool parse_constraint(Node& node, Lexer<ExpressionLexer>& lexer, TypeConstraint& out) {
            if (!parse_constraint_and(node, lexer, out)) {
                return false;
            }
            while (true) {
                if (!skip_gap(node, lexer)) {
                    return false;
                }
                auto t = lexer.look_at();
                if (!is_word(t, "or")) {
                    return true;
                }
                lexer.consume();
                TypeConstraint right;
                if (!parse_constraint_and(node, lexer, right)) {
                    return false;
                }
                TypeConstraint combined;
                combined.op = TypeConstraint::Op::OR;
                combined.operands.push_back(std::move(out));
                combined.operands.push_back(std::move(right));
                out = std::move(combined);
            }
        }

        bool parse_constraint_list(Node& node, Lexer<ExpressionLexer>& lexer, TypeName& type) {
            if (!skip_gap(node, lexer)) {
                return false;
            }
            auto t = lexer.look_at();
            if (t.type != TokenType::OpeningSquareBracket) {
                return true;
            }
            lexer.consume();
            TypeConstraint constraint;
            if (!parse_constraint(node, lexer, constraint)) {
                return false;
            }
            if (!skip_gap(node, lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type != TokenType::ClosingSquareBracket) {
                node.report_error(ErrorType::SYNTAX, "expected `]` after constraint on `%s`, but got `%s`", type.name, t.view());
                return false;
            }
            lexer.consume();
            type.constraint = std::move(constraint);
            return true;
        }

    }  // namespace

    bool parse_type_ref(Node& node, Lexer<ExpressionLexer>& lexer, TypeName& out) {
        auto t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            node.report_error(ErrorType::SYNTAX, "expected a type name, but got `%s`", t.view());
            return false;
        }
        const char* begin = t.begin;
        const char* end = t.end;
        lexer.consume();

        while (true) {
            auto dot = lexer.look_at();
            if (dot.is_error()) {
                node.report_error(ErrorType::SYNTAX, "%s", get_error_token_description(dot.type));
                return false;
            }
            if (dot.type != TokenType::Dot || dot.begin != end) {
                break;
            }
            lexer.consume();
            auto part = lexer.look_at();
            if (part.is_error()) {
                node.report_error(ErrorType::SYNTAX, "%s", get_error_token_description(part.type));
                return false;
            }
            if (part.type != TokenType::BareWord || part.begin != dot.end) {
                node.report_error(ErrorType::SYNTAX, "expected a type name, but got `%s`", part.view());
                return false;
            }
            end = part.end;
            lexer.consume();
        }

        out = TypeName{};
        out.name = std::string_view(begin, static_cast<size_t>(end - begin));
        if (!skip_gap(node, lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type == TokenType::Less) {
            lexer.consume();
            if (!skip_gap(node, lexer)) {
                return false;
            }
            while (true) {
                t = lexer.look_at();
                if (t.type == TokenType::Greater) {
                    break;
                }
                TypeName arg;
                if (!parse_type_ref(node, lexer, arg)) {
                    return false;
                }
                out.arguments.push_back(std::move(arg));
                if (!skip_gap(node, lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type == TokenType::Comma) {
                    lexer.consume();
                    if (!skip_gap(node, lexer)) {
                        return false;
                    }
                    continue;
                }
                if (t.type == TokenType::Greater) {
                    break;
                }
                node.report_error(ErrorType::SYNTAX, "expected `,` or `>` in type `%s`, but got `%s`", out.name, t.view());
                return false;
            }
            if (out.arguments.empty()) {
                node.report_error(ErrorType::SYNTAX, "type `%s` requires type arguments", out.name);
                return false;
            }
            if ((out.name == "list" || out.name == "set") && out.arguments.size() != 1) {
                node.report_error(ErrorType::SYNTAX, "type `%s` requires 1 type argument", out.name);
                return false;
            }
            if (out.name == "map" && out.arguments.size() != 2) {
                node.report_error(ErrorType::SYNTAX, "type `map` requires 2 type arguments");
                return false;
            }
            lexer.consume();
        }
        return parse_constraint_list(node, lexer, out);
    }

    std::string format_type_ref(const TypeName& ref) {
        std::string out;
        append_type_name(out, ref);
        return out;
    }

    bool check_constraint(const TypeConstraint& constraint, const TypeName& argument) {
        switch (constraint.op) {
            case TypeConstraint::Op::CALL:
                return true;
            case TypeConstraint::Op::NOT:
                if (constraint.operands.empty()) {
                    return true;
                }
                return check_constraint(constraint.operands[0], argument);
            case TypeConstraint::Op::AND:
            case TypeConstraint::Op::OR:
                if (constraint.operands.size() < 2) {
                    return true;
                }
                return check_constraint(constraint.operands[0], argument)
                    && check_constraint(constraint.operands[1], argument);
        }
        return true;
    }

}  // namespace shark::parser
