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

#include <shark/parser/alias.h>
#include <shark/parser/file.h>
#include <shark/parser/pool.h>

namespace shark::parser {

    bool AliasParser::bind_alias() {
        if (bind) {
            return true;
        }
        SymbolNode* symbol = file->find_type(target_name);
        if (symbol == nullptr) {
            report_error(ErrorType::SEMANTIC, "alias `%s` target `%s` was not found", name, target_name);
            return false;
        }
        target = symbol;
        bind = true;
        if (!emit_to_pool()) {
            return false;
        }
        return true;
    }

    bool AliasParser::skip_gap(Lexer<ExpressionLexer>& lexer) {
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

    bool AliasParser::parse_target_name(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a symbol name, but got `%s`", t.view());
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
                report_error(ErrorType::SYNTAX, "expected a symbol name, but got `%s`", part.view());
                return false;
            }
            end = part.end;
            lexer.consume();
        }

        target_name = std::string_view(begin, static_cast<size_t>(end - begin));
        return true;
    }

    bool AliasParser::finish(Lexer<ExpressionLexer>& lexer) {
        if (!skip_gap(lexer)) {
            return false;
        }
        auto t = lexer.look_at();
        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        tailing_comment = pool->create_node<CommentParser>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "%s `%s` = `%s`", is_using ? "using" : "typedef", name, target_name);
            return false;
        }
        report_debug(ErrorType::NONE, "%s `%s` = `%s`", is_using ? "using" : "typedef", name, target_name);
        return true;
    }

    bool AliasParser::parse_using(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        is_using = true;
        start_location = {t.line, t.column};
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected an alias name, but got `%s`", t.view());
            return false;
        }
        name = t.view();
        lexer.consume();

        if (!skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::Equals) {
            report_error(ErrorType::SYNTAX, "expected `=`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();

        if (!skip_gap(lexer) || !parse_target_name(lexer)) {
            return false;
        }
        return finish(lexer);
    }

    bool AliasParser::parse_typedef(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        is_using = false;
        start_location = {t.line, t.column};
        lexer.consume();

        if (!skip_gap(lexer) || !parse_target_name(lexer) || !skip_gap(lexer)) {
            return false;
        }
        t = lexer.look_at();
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected an alias name, but got `%s`", t.view());
            return false;
        }
        name = t.view();
        lexer.consume();
        return finish(lexer);
    }

    bool AliasParser::parse(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type == TokenType::BareWord && t == kUsingKey) {
            return parse_using(lexer);
        }
        if (t.type == TokenType::BareWord && t == kTypeDefKey) {
            return parse_typedef(lexer);
        }
        report_error(ErrorType::SYNTAX, "expected `using` or `typedef`, but got `%s`", t.view());
        return false;
    }

} // namespace shark::parser
