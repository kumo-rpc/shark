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

#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/file.h>
#include <shark/parser/servlet.h>
#include <shark/parser/type_name.h>

namespace shark::parser {

    ServletParser::BindStep ServletParser::bind_servlet() {
        if (bind) {
            return BindStep::DONE;
        }
        std::vector<const ServletParser*> resolved;
        bool waiting = false;
        for (const TypeName& base_name : base_names) {
            SymbolNode* symbol = file->find_type(base_name);
            if (symbol == nullptr) {
                report_error(ErrorType::SEMANTIC, "servlet `%s` base `%s` was not found", name, format_type_ref(base_name).c_str());
                return BindStep::FAIL;
            }
            if (symbol->node_type != SymbolType::SERVLET) {
                report_error(ErrorType::SEMANTIC, "servlet `%s` base `%s` is not a servlet", name, format_type_ref(base_name).c_str());
                return BindStep::FAIL;
            }
            auto* base = static_cast<ServletParser*>(symbol);
            if (!base->bind) {
                waiting = true;
                continue;
            }
            resolved.push_back(base);
        }
        if (waiting) {
            return BindStep::WAIT;
        }
        bases = std::move(resolved);
        bind = true;
        if (!emit_to_pool()) {
            report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", full_name);
            return BindStep::FAIL;
        }
        return BindStep::DONE;
    }

    bool ServletParser::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool ServletParser::parse_base_name(Lexer<ExpressionLexer>& lexer) {
        TypeName base_name;
        if (!parse_type_ref(*this, lexer, base_name)) {
            return false;
        }
        base_names.push_back(std::move(base_name));
        return true;
    }

    bool ServletParser::parse_function(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentParser*>& comments,
        std::vector<AnnotationParser*>& annotations) {
        auto* function = pool->create_symbol<FunctionParser>(file);
        function->servlet = this;
        function->leading_comment = std::move(comments);
        function->annotations = std::move(annotations);
        if (!function->parse(lexer)) {
            return false;
        }
        method.push_back(function);
        return true;
    }

    bool ServletParser::parse(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type != TokenType::BareWord || t != kServletKey) {
            report_error(ErrorType::SYNTAX, "expected `servlet`, but got `%s`", t.view());
            return false;
        }
        start_location = {t.line, t.column};
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        if (!parse_type_ref(*this, lexer, declared_name)) {
            return false;
        }
        name = declared_name.name;

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type == TokenType::Colon) {
            lexer.consume();
            if (!skip_gap(lexer)) {
                return false;
            }
            while (true) {
                if (!parse_base_name(lexer) || !skip_gap(lexer)) {
                    return false;
                }
                t = lexer.look_at();
                if (t.type == TokenType::Comma) {
                    lexer.consume();
                    if (!skip_gap(lexer)) {
                        return false;
                    }
                    t = lexer.look_at();
                    if (t.type == TokenType::OpeningCurlyBrace) {
                        break;
                    }
                    continue;
                }
                break;
            }
        }

        t = lexer.look_at();
        if (t.type != TokenType::OpeningCurlyBrace) {
            report_error(ErrorType::SYNTAX, "expected `{`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        while (true) {
            std::vector<CommentParser*> comments;
            std::vector<AnnotationParser*> annotations;
            while (true) {
                auto* comment = pool->create_node<CommentParser>(file);
                if (!comment->parse_leading(lexer)) {
                    return false;
                }
                if (!comment->empty()) {
                    comments.push_back(comment);
                }

                t = lexer.look_at();
                if (t.is_error()) {
                    report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                    return false;
                }
                if (t.type == TokenType::BareWord && t == kAnnotationKey) {
                    auto* annotation = pool->create_node<AnnotationParser>(file);
                    if (!annotation->parse(lexer)) {
                        return false;
                    }
                    if (annotation->file_level) {
                        annotation->report_warn(
                            ErrorType::SYNTAX,
                            "file-level annotation belongs in the file scope, not inside an object definition");
                        file->annotations.push_back(annotation);
                    } else {
                        annotations.push_back(annotation);
                    }
                    continue;
                }
                break;
            }

            if (t.type == TokenType::ClosingCurlyBrace) {
                break;
            }
            if (t.type == TokenType::BareWord && t == kFunctionKey) {
                if (!parse_function(lexer, comments, annotations)) {
                    return false;
                }
                continue;
            }

            report_error(ErrorType::SYNTAX, "expected `function`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        tailing_comment = pool->create_node<CommentParser>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "servlet `%s` bases `%zu` functions `%zu`", name, base_names.size(), method.size());
            return false;
        }

        t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type == TokenType::Semicolon) {
            lexer.consume();
            if (!tailing_comment->parse_tailing(lexer)) {
                report_debug(ErrorType::NONE, "servlet `%s` bases `%zu` functions `%zu`", name, base_names.size(), method.size());
                return false;
            }
        }

        report_debug(ErrorType::NONE, "servlet `%s` bases `%zu` functions `%zu`", name, base_names.size(), method.size());
        return true;
    }

} // namespace shark::parser
