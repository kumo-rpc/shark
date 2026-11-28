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

#include <shark/parser/enum.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/servlet.h>
#include <shark/parser/struct.h>
#include <shark/parser/type_name.h>

namespace shark::parser {

    bool in_scope(FunctionParser& function, std::string_view name) {
        for (TypeName& parameter : function.parameters) {
            if (parameter.name != name) {
                continue;
            }
            parameter.is_parameter = true;
            return true;
        }
        if (function.servlet != nullptr) {
            for (const TypeName& parameter : function.servlet->declared_name.arguments) {
                if (parameter.name == name) {
                    return true;
                }
            }
        }
        if (function.container == nullptr) {
            return false;
        }
        for (const TypeName& parameter : function.container->declared_name.arguments) {
            if (parameter.name == name) {
                return true;
            }
        }
        return false;
    }

    bool contains_parameter(const TypeName& type) {
        if (type.is_parameter) {
            return true;
        }
        for (const TypeName& argument : type.arguments) {
            if (contains_parameter(argument)) {
                return true;
            }
        }
        return false;
    }

    void mark_scope_parameters(FunctionParser& function, TypeName& type_name) {
        if (type_name.arguments.empty()) {
            if (in_scope(function, type_name.name)) {
                type_name.is_parameter = true;
            }
            return;
        }
        for (TypeName& argument : type_name.arguments) {
            mark_scope_parameters(function, argument);
        }
    }

    bool FunctionParser::bind_named_type(TypeName& type_name, bool allow_void, SymbolNode*& resolved) {
        mark_scope_parameters(*this, type_name);
        resolved = nullptr;
        if (type_name.is_parameter) {
            return true;
        }
        SymbolNode* symbol = file->find_type(type_name);
        if (symbol != nullptr) {
            symbol = file->concrete_type(symbol);
        }
        if (symbol == nullptr) {
            report_error(ErrorType::SEMANTIC, "function `%s` type `%s` was not found", name, type_name.name);
            return false;
        }
        if (!allow_void && symbol->name == "void") {
            report_error(ErrorType::SEMANTIC, "function `%s` parameter type `void` is not allowed", name);
            return false;
        }
        if (symbol->node_type == SymbolType::STRUCT) {
            auto* type = static_cast<StructParser*>(symbol);
            if (!(type->builtin || type->bind)) {
                report_error(ErrorType::SEMANTIC, "function `%s` type `%s` was not bound", name, type_name.name);
                return false;
            }
        } else if (symbol->node_type == SymbolType::ENUM) {
            if (!symbol->bind) {
                report_error(ErrorType::SEMANTIC, "function `%s` type `%s` was not bound", name, type_name.name);
                return false;
            }
        } else if (symbol->node_type != SymbolType::PRIMITIVE) {
            report_error(ErrorType::SEMANTIC, "function `%s` type `%s` was not bound", name, type_name.name);
            return false;
        }
        resolved = symbol;
        return true;
    }

    bool FunctionParser::bind_function() {
        SymbolNode* resolved = nullptr;
        if (!bind_named_type(return_type, true, resolved)) {
            return false;
        }
        ret_type = resolved;
        for (std::size_t i = 0; i < params.size(); ++i) {
            for (std::size_t j = i + 1; j < params.size(); ++j) {
                if (params[i]->id != params[j]->id) {
                    continue;
                }
                report_error(ErrorType::SEMANTIC, "parameter `%s` number `%u` duplicates parameter `%s`", params[j]->name, static_cast<unsigned>(params[j]->id), params[i]->name);
                return false;
            }
        }
        for (FieldParser* param : params) {
            SymbolNode* param_type = nullptr;
            if (!bind_named_type(param->type_ref, false, param_type)) {
                return false;
            }
            param->type = param_type;
            if (contains_parameter(param->type_ref)) {
                param->generic = true;
            }
            param->bind = true;
            if (!param->emit_to_pool()) {
                return false;
            }
        }
        bind = true;
        if (!emit_to_pool()) {
            return false;
        }
        return true;
    }

    bool FunctionParser::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool FunctionParser::parse_parameters(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.type != TokenType::Less) {
            return true;
        }
        lexer.consume();
        if (!skip_gap(lexer)) {
            return false;
        }
        while (true) {
            t = lexer.look_at();
            if (t.type == TokenType::Greater) {
                break;
            }
            TypeName parameter;
            if (!parse_type_ref(*this, lexer, parameter)) {
                return false;
            }
            parameters.push_back(std::move(parameter));
            if (!skip_gap(lexer)) {
                return false;
            }
            t = lexer.look_at();
            if (t.type == TokenType::Comma) {
                lexer.consume();
                if (!skip_gap(lexer)) {
                    return false;
                }
                continue;
            }
            if (t.type == TokenType::Greater) {
                break;
            }
            report_error(ErrorType::SYNTAX, "expected `,` or `>` in function `%s`, but got `%s`", name, t.view());
            return false;
        }
        if (parameters.empty()) {
            report_error(ErrorType::SYNTAX, "function type parameter list is empty");
            return false;
        }
        lexer.consume();
        return true;
    }

    bool FunctionParser::parse(Lexer<ExpressionLexer>& lexer) {
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

        if (!skip_gap(lexer) || !parse_parameters(lexer) || !skip_gap(lexer) || !parse_type_ref(*this, lexer, return_type) || !skip_gap(lexer)) {
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
                auto* param = pool->create_symbol<FieldParser>(file);
                if (!param->parse(lexer, FieldParser::End::PARAMETER)) {
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

        tailing_comment = pool->create_node<CommentParser>(file);
        const std::string shown = format_type_ref(return_type);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "function `%s` %s params `%zu`", name, shown.c_str(), params.size());
            return false;
        }

        report_debug(ErrorType::NONE, "function `%s` %s params `%zu`", name, shown.c_str(), params.size());
        return true;
    }

} // namespace shark::parser
