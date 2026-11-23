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
#include <shark/parser/import.h>

#include "pool.h"

namespace shark::parser {

    bool ImportNode::parse(Lexer<ExpressionLexer>& lexer) {
        // Parse leading comments.
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::BareWord || t != kImportKey) {
            report_error(ErrorType::SYNTAX, "expected `import`, but got `%s`", t.view());
            return false;
        }

        start_location = {t.line, t.column};

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse optional import classification.
        classify = { };

        if (t == ".") {
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            if (t.type != TokenType::BareWord) {
                report_error(ErrorType::SYNTAX, "expected an import classification, but got `%s`", t.view());
                return false;
            }

            classify = t.view();

            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();
        }

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        // Parse import path.
        if (t.type != TokenType::StringLiteral && t.type != TokenType::QuotedIdentifier) {
            report_error(
                ErrorType::SYNTAX,
                "expected an import path in the form `import \"path\"` or `import 'path'`, but got `%s`",
                t.view());
            return false;
        }

        path = t.view();

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t != ";") {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();

        // Parse trailing comments.
        tailing_comment = pool->create_node<CommentNode>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            report_debug(ErrorType::NONE, "import `%s` classify `%s`", path, classify);
            return false;
        }

        report_debug(ErrorType::NONE, "import `%s` classify `%s`", path, classify);
        return true;
    }

} // namespace shark::parser
