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


#include <shark/parser/comment.h>
#include <turbo/strings/match.h>

namespace shark::parser {

    bool CommentNode::is_tailing_prefix(std::string_view comment) {
        if (turbo::starts_with(comment, "//!") ||
                turbo::starts_with(comment, "//<") ||
                turbo::starts_with(comment, "///!") ||
                turbo::starts_with(comment, "///<") ||
                turbo::starts_with(comment, "/*!") ||
                turbo::starts_with(comment, "/*<") ||
                turbo::starts_with(comment, "/**<") ||
                turbo::starts_with(comment, "/**!")
                ) {
                return true;
                }
        return false;
    }
    bool CommentNode::parse_tailing(Lexer<ExpressionLexer> &lexer,
        const std::function<bool(std::string_view)> & func) {
        /// advance white space
        auto t = lexer.look_at();
        std::string white;
        while (t.type == TokenType::Whitespace) {
            lexer.consume();
            white.append(t.begin, t.end);
            t = lexer.look_at();
        }
        if (t.is_error()) {
            report_error(ErrorType::COMMENT, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type == TokenType::EndOfStream) {
            return true;
        }

        auto pos = white.find_first_of('\n');
        if (pos != std::string::npos) {
            is_tailing = false;
        } else {
            std::string_view com(t.begin, t.end - t.begin);
            is_tailing = func(com);
        }


        if (!is_tailing) {
            return true;
        }

        if (t.type == TokenType::Comment) {
            start_location = {t.line, t.column};
            content.push_back( t);
            lexer.consume();
            t = lexer.look_at();
        }
        return true;
    }

    bool CommentNode::parse_leading(Lexer<ExpressionLexer> &lexer,
        const std::function<bool(std::string_view)> & func) {
        /// advance white space
        auto t = lexer.look_at();
        std::string white;
        while (t.type == TokenType::Whitespace) {
            lexer.consume();
            white.append(t.begin, t.end);
            t = lexer.look_at();
        }
        if (t.is_error()) {
            report_error(ErrorType::COMMENT, "%s", get_error_token_description(t.type));
            return false;
        }


        if (t.type == TokenType::EndOfStream) {
            return true;
        }


        auto pos = white.find_first_of('\n');
        if (pos != std::string::npos) {
            is_tailing = false;
        } else {
            std::string_view com(t.begin, t.end - t.begin);
            is_tailing = func(com);
        }
        if (is_tailing) {
            return true;
        }

        while (t.type == TokenType::Comment) {
            if (content.empty()) {
                start_location = {t.line, t.column};
            }
            content.push_back( t);
            lexer.consume();
            t = lexer.look_at();
            while(t.type == TokenType::Whitespace) {
                lexer.consume();
                t = lexer.look_at();
            }
        }
        if (t.is_error()) {
            report_error(ErrorType::COMMENT, "%s", get_error_token_description(t.type));
            return false;
        }
        return true;
    }
}  // namespace shark::parser
