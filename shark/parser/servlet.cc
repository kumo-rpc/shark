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
#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/file.h>
#include <shark/parser/servlet.h>

namespace shark::parser {

    bool ServletNode::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool ServletNode::parse_base_name(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a base name, but got `%s`", t.view());
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
                report_error(ErrorType::SYNTAX, "expected a base name, but got `%s`", part.view());
                return false;
            }
            end = part.end;
            lexer.consume();
        }

        base_names.emplace_back(begin, static_cast<size_t>(end - begin));
        return true;
    }

    bool ServletNode::parse_function(Lexer<ExpressionLexer>& lexer,
        std::vector<CommentNode*>& comments,
        std::vector<AnnotationNode*>& annotations) {
        auto* function = pool->create_symbol<FunctionNode>(file);
        function->leading_comment = std::move(comments);
        function->annotations = std::move(annotations);
        if (!function->parse(lexer)) {
            return false;
        }
        method.push_back(function);
        return true;
    }

    bool ServletNode::parse(Lexer<ExpressionLexer>& lexer) {
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
        t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a servlet name, but got `%s`", t.view());
            return false;
        }
        name = t.view();
        lexer.consume();

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
            std::vector<CommentNode*> comments;
            std::vector<AnnotationNode*> annotations;
            while (true) {
                auto* comment = pool->create_node<CommentNode>(file);
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
                    auto* annotation = pool->create_node<AnnotationNode>(file);
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

        tailing_comment = pool->create_node<CommentNode>(file);
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
