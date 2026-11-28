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

#include <shark/parser/primitive.h>
#include <shark/parser/builtin.h>
#include <shark/schema/schema_defines.h>

#include <optional>
#include <string_view>
#include <variant>

namespace shark::parser {

    bool PrimitiveParser::parse(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type != TokenType::BareWord || t != kTypeKey) {
            report_error(ErrorType::SYNTAX, "expected `type`, but got `%s`", t.view());
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
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected a type name, but got `%s`", t.view());
            return false;
        }
        name = t.view();
        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }
        if (t.type != TokenType::Semicolon) {
            report_error(ErrorType::SYNTAX, "expected `;`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();
        for (AnnotationParser* annotation : annotations) {
            if (annotation == nullptr || annotation->classify != "builtin" || annotation->key != "phy_type") {
                continue;
            }
            const auto* text = annotation->object == nullptr ? nullptr : std::get_if<std::string_view>(&annotation->object->value);
            if (text == nullptr || text->size() < 2) {
                report_error(ErrorType::SEMANTIC, "type `%s` physical type is missing", name);
                return false;
            }
            std::optional<PhysicalType> phy = physical_type(text->substr(1, text->size() - 2));
            if (!phy.has_value()) {
                report_error(ErrorType::SEMANTIC, "type `%s` physical type is unknown", name);
                return false;
            }
            phy_type = *phy;
        }
        return true;
    }

}  // namespace shark::parser
