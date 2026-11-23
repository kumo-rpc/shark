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
#include <shark/parser/edition.h>

namespace shark::parser {

    bool Edition::parse(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::BareWord || t != kEditionKey) {
            report_error(ErrorType::SYNTAX, "expected to got edition `%s`, but got `%s`", kEditionKey, t.view());
            return false;
        }
        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t != "=") {
            report_error(ErrorType::SYNTAX, "expected to got edition `=`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::StringLiteral && t.type != TokenType::QuotedIdentifier) {
            report_error(
                ErrorType::SYNTAX,
                "expected an edition identifier in the form `edition = 'sk2026'` or `edition = \"sk2026\"`, but got `%s`",
                t.view());
            return false;
        }

        edition = t.view();
        lexer.consume();

        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t != ";") {
            report_error(ErrorType::SYNTAX, "expected to got edition `;`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        return true;
    }
} // namespace shark::parser
