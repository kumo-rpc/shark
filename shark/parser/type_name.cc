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

    TypeName::TypeName() = default;
    TypeName::~TypeName() = default;
    TypeName::TypeName(const TypeName& other) = default;
    TypeName& TypeName::operator=(const TypeName& other) = default;
    TypeName::TypeName(TypeName&& other) noexcept = default;
    TypeName& TypeName::operator=(TypeName&& other) noexcept = default;

    namespace {

        void append_type_ref(std::string& out, const TypeRef& ref) {
            if (const auto* plain = std::get_if<std::string_view>(&ref)) {
                out.append(plain->data(), plain->size());
                return;
            }
            const TypeName& type = std::get<TypeName>(ref);
            out.append(type.name.data(), type.name.size());
            out.push_back('<');
            for (std::size_t i = 0; i < type.args.size(); ++i) {
                if (i != 0) {
                    out.append(", ");
                }
                append_type_ref(out, type.args[i].value);
            }
            out.push_back('>');
        }

    }  // namespace

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

    }  // namespace

    bool parse_type_ref(Node& node, Lexer<ExpressionLexer>& lexer, TypeRef& out) {
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

        std::string_view name(begin, static_cast<size_t>(end - begin));
        if (!skip_gap(node, lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::Less) {
            out = name;
            return true;
        }
        lexer.consume();
        if (!skip_gap(node, lexer)) {
            return false;
        }

        TypeName type;
        type.name = name;
        while (true) {
            t = lexer.look_at();
            if (t.type == TokenType::Greater) {
                break;
            }
            TypeRef arg;
            if (!parse_type_ref(node, lexer, arg)) {
                return false;
            }
            type.args.push_back(TypeName::Arg{std::move(arg)});
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
            node.report_error(ErrorType::SYNTAX, "expected `,` or `>` in type `%s`, but got `%s`", name, t.view());
            return false;
        }
        if (type.args.empty()) {
            node.report_error(ErrorType::SYNTAX, "type `%s` requires type arguments", name);
            return false;
        }
        if ((name == "list" || name == "set") && type.args.size() != 1) {
            node.report_error(ErrorType::SYNTAX, "type `%s` requires 1 type argument", name);
            return false;
        }
        if (name == "map" && type.args.size() != 2) {
            node.report_error(ErrorType::SYNTAX, "type `map` requires 2 type arguments");
            return false;
        }
        lexer.consume();
        out = std::move(type);
        return true;
    }

    std::string format_type_ref(const TypeRef& ref) {
        std::string out;
        append_type_ref(out, ref);
        return out;
    }

}  // namespace shark::parser
