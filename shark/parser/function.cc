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
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>
#include <shark/parser/type_name.h>

namespace shark::parser {

    bool FunctionNode::bind_named_type(std::string_view type_name, bool allow_void, const StructNode*& struct_type, const EnumNode*& enum_type) {
        if (type_name == "void") {
            if (!allow_void) {
                report_error(ErrorType::SEMANTIC, "function `%s` parameter type `void` is not allowed", name);
                return false;
            }
            struct_type = nullptr;
            enum_type = nullptr;
            return true;
        }
        SymbolNode* symbol = file->find_type(type_name);
        if (symbol != nullptr) {
            symbol = file->concrete_type(symbol);
        }
        if (symbol == nullptr) {
            report_error(ErrorType::SEMANTIC, "function `%s` type `%s` was not found", name, type_name);
            return false;
        }
        if (symbol->node_type == NodeType::STRUCT) {
            auto* type = static_cast<StructNode*>(symbol);
            if (type->builtin || type->bind) {
                struct_type = type;
                enum_type = nullptr;
                return true;
            }
        } else if (symbol->node_type == NodeType::ENUM) {
            auto* type = static_cast<EnumNode*>(symbol);
            if (type->bind) {
                struct_type = nullptr;
                enum_type = type;
                return true;
            }
        }
        report_error(ErrorType::SEMANTIC, "function `%s` type `%s` was not bound", name, type_name);
        return false;
    }

    bool FunctionNode::bind_function() {
        const StructNode* struct_type = nullptr;
        const EnumNode* enum_type = nullptr;
        if (!bind_named_type(return_type_name, true, struct_type, enum_type)) {
            return false;
        }
        ret_type = struct_type;
        ret_enum = enum_type;
        for (FieldNode* param : params) {
            const StructNode* param_struct = nullptr;
            const EnumNode* param_enum = nullptr;
            if (!bind_named_type(type_ref_name(param->type_ref), false, param_struct, param_enum)) {
                return false;
            }
            param->type = param_struct;
            param->enum_type = param_enum;
            param->bind = true;
        }
        bind = true;
        return true;
    }

    bool FunctionNode::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool FunctionNode::parse_type_name(Lexer<ExpressionLexer>& lexer) {
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

        return_type_name = std::string_view(begin, static_cast<size_t>(end - begin));
        return true;
    }

    bool FunctionNode::parse(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type != TokenType::BareWord || t != kFunctionKey) {
            report_error(ErrorType::SYNTAX, "expected `function`, but got `%s`", t.view());
            return false;
        }
        start_location = {t.line, t.column};
        lexer.consume();

        if (!skip_gap(lexer) || !parse_type_name(lexer) || !skip_gap(lexer)) {
            return false;
        }

        t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a function name, but got `%s`", t.view());
            return false;
        }
        name = t.view();
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::OpeningRoundBracket) {
            report_error(ErrorType::SYNTAX, "expected `(`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::ClosingRoundBracket) {
            while (true) {
                auto* param = pool->create_symbol<FieldNode>(file);
                if (!param->parse(lexer, FieldNode::End::PARAMETER)) {
                    return false;
                }
                params.push_back(param);

                if (!skip_gap(lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type == TokenType::Comma) {
                    lexer.consume();
                    if (!skip_gap(lexer)) {
                        return false;
                    }
                    t = lexer.look_at();
                    if (t.type == TokenType::ClosingRoundBracket) {
                        break;
                    }
                    continue;
                }
                if (t.type == TokenType::ClosingRoundBracket) {
                    break;
                }
                report_error(ErrorType::SYNTAX, "expected `,` or `)`, but got `%s`", t.view());
                return false;
            }
        }

        lexer.consume();
        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        tailing_comment = pool->create_node<CommentNode>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "function `%s` %s params `%zu`", name, return_type_name, params.size());
            return false;
        }

        report_debug(ErrorType::NONE, "function `%s` %s params `%zu`", name, return_type_name, params.size());
        return true;
    }

} // namespace shark::parser
