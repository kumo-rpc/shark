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

#include <shark/parser/annotation.h>
#include <shark/parser/const_value.h>
#include <shark/parser/enum.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>

#include <shark/parser/constants.h>

#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace shark::parser {

    bool AnnotationNode::parse_one(
        Lexer<ExpressionLexer>& lexer) {

        auto t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type == TokenType::StringLiteral || t.type == TokenType::QuotedIdentifier) {
            payload.emplace_back(t.view());
            lexer.consume();
            return true;
        }

        if (t.type == TokenType::BareWord && (t == "true" || t == "false")) {
            payload.emplace_back(t == "true");
            lexer.consume();
            return true;
        }

        if (t.type != TokenType::Number) {
            report_error(ErrorType::SYNTAX, "expected an annotation value, but got `%s`", t.view());
            return false;
        }

        auto value = t.view();
        std::string number(value);
        char* end = nullptr;

        // Determine whether the number is an integer or a floating-point value.
        bool is_hex = value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X');

        bool is_float = is_hex
            ? value.find_first_of(".pP") != std::string_view::npos
            : value.find_first_of(".eE") != std::string_view::npos;

        errno = 0;

        if (is_float) {
            float result = std::strtof(number.c_str(), &end);

            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid floating-point annotation value `%s`", t.view());
                return false;
            }

            payload.emplace_back(result);
        } else {
            int64_t result = std::strtoll(number.c_str(), &end, is_hex ? 16 : 10);

            if (errno == ERANGE || end != number.c_str() + number.size()) {
                report_error(ErrorType::SYNTAX, "invalid integer annotation value `%s`", t.view());
                return false;
            }

            payload.emplace_back(result);
        }

        lexer.consume();
        return true;
    }

    namespace {

        bool unsupported_width(const TypeRef& ref) {
            const std::string_view name = type_ref_name(ref);
            if (name == "int128" || name == "uint128") {
                return true;
            }
            const auto* type = std::get_if<TypeName>(&ref);
            if (type == nullptr) {
                return false;
            }
            for (const TypeName::Arg& arg : type->args) {
                if (unsupported_width(arg.value)) {
                    return true;
                }
            }
            return false;
        }

        void append_const(std::string& out, const ConstValue* value) {
            if (value == nullptr) {
                return;
            }
            if (const auto* flag = std::get_if<bool>(&value->value)) {
                out.append(*flag ? "true" : "false");
            } else if (const auto* number = std::get_if<int64_t>(&value->value)) {
                out.append(std::to_string(*number));
            } else if (const auto* number = std::get_if<double>(&value->value)) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%g", *number);
                out.append(buf);
            } else if (const auto* text = std::get_if<std::string_view>(&value->value)) {
                out.append(text->data(), text->size());
            } else if (const auto* id = std::get_if<ConstValue::Identifier>(&value->value)) {
                out.append(id->name.data(), id->name.size());
            } else if (const auto* list = std::get_if<ConstValue::List>(&value->value)) {
                out.push_back('[');
                for (std::size_t i = 0; i < list->elements.size(); ++i) {
                    if (i != 0) {
                        out.append(", ");
                    }
                    append_const(out, list->elements[i]);
                }
                out.push_back(']');
            } else if (const auto* map = std::get_if<ConstValue::Map>(&value->value)) {
                out.push_back('{');
                for (std::size_t i = 0; i < map->entries.size(); ++i) {
                    if (i != 0) {
                        out.append(", ");
                    }
                    append_const(out, map->entries[i].first);
                    out.append(" = ");
                    append_const(out, map->entries[i].second);
                }
                out.push_back('}');
            }
        }

        void append_payload(std::string& out, const AnnotationNode::value_type& value) {
            if (const auto* text = std::get_if<std::string_view>(&value)) {
                out.append(text->data(), text->size());
            } else if (const auto* number = std::get_if<int64_t>(&value)) {
                out.append(std::to_string(*number));
            } else if (const auto* number = std::get_if<float>(&value)) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(*number));
                out.append(buf);
            } else if (const auto* flag = std::get_if<bool>(&value)) {
                out.append(*flag ? "true" : "false");
            }
        }

        std::size_t const_size(const ConstValue* value) {
            if (value == nullptr) {
                return 0;
            }
            if (const auto* list = std::get_if<ConstValue::List>(&value->value)) {
                return list->elements.size();
            }
            if (const auto* map = std::get_if<ConstValue::Map>(&value->value)) {
                return map->entries.size();
            }
            return 1;
        }

        std::string format_payload(const std::vector<AnnotationNode::value_type>& payload) {
            std::string out;
            if (payload.size() == 1) {
                append_payload(out, payload[0]);
                return out;
            }
            out.push_back('[');
            for (std::size_t i = 0; i < payload.size(); ++i) {
                if (i != 0) {
                    out.append(", ");
                }
                append_payload(out, payload[i]);
            }
            out.push_back(']');
            return out;
        }

    }  // namespace

    bool AnnotationNode::parse_typed_value(Lexer<ExpressionLexer>& lexer) {
        auto t = lexer.look_at();
        if (t.type != TokenType::Less) {
            report_error(ErrorType::SYNTAX, "expected `<`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();
        lexer.consume_white_space(true);
        if (!parse_type_ref(*this, lexer, type_ref)) {
            return false;
        }
        if (unsupported_width(type_ref)) {
            report_error(ErrorType::SYNTAX, "annotation type `%s` is wider than int64", format_type_ref(type_ref).c_str());
            return false;
        }
        lexer.consume_white_space(true);
        t = lexer.look_at();
        if (t.type != TokenType::Greater) {
            report_error(ErrorType::SYNTAX, "expected `>`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();
        if (t.type != TokenType::Equals) {
            report_error(ErrorType::SYNTAX, "expected `=`, but got `%s`", t.view());
            return false;
        }
        lexer.consume();
        object = pool->create_node<ConstValue>(file);
        return object->parse(lexer);
    }

    bool AnnotationNode::parse(
        Lexer<ExpressionLexer>& lexer) {

        auto t = lexer.look_at();
        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type != TokenType::BareWord || t != kAnnotationKey) {
            report_error(ErrorType::SYNTAX, "expected `annotation`, but got `%s`", t.view());
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

        file_level = false;
        if (t.type == TokenType::At) {
            file_level = true;
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }
        }

        // Parse optional classification.
        classify = { };

        if (t.type == TokenType::Dot) {
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            if (t.type != TokenType::BareWord) {
                report_error(ErrorType::SYNTAX, "expected an annotation classification, but got `%s`", t.view());
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

        // Parse annotation key.
        if (t.type != TokenType::BareWord) {
            report_error(ErrorType::SYNTAX, "expected an annotation key, but got `%s`", t.view());
            return false;
        }

        key = t.view();

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        if (t.type == TokenType::Less) {
            typed = true;
            if (!parse_typed_value(lexer)) {
                return false;
            }
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
            tailing_comment = pool->create_node<CommentNode>(file);
            if (!tailing_comment->parse_tailing(lexer)) {
                return false;
            }
            std::string shown;
            append_const(shown, object);
            report_debug(ErrorType::NONE, "annotation `%s` classify `%s` type `%s` value size `%zu` value `%s`", key, classify, format_type_ref(type_ref).c_str(), const_size(object), shown.c_str());
            return true;
        }

        if (t.type != TokenType::Equals) {
            report_error(ErrorType::SYNTAX, "expected `=`, but got `%s`", t.view());
            return false;
        }

        lexer.consume();
        lexer.consume_white_space(true);
        t = lexer.look_at();

        if (t.is_error()) {
            report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
            return false;
        }

        payload.clear();

        // Parse a single value or an array.
        if (t.type == TokenType::OpeningSquareBracket) {
            lexer.consume();
            lexer.consume_white_space(true);
            t = lexer.look_at();

            if (t.is_error()) {
                report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                return false;
            }

            if (t.type != TokenType::ClosingSquareBracket) {
                while (true) {
                    if (!parse_one(lexer)) {
                        return false;
                    }

                    lexer.consume_white_space(true);
                    t = lexer.look_at();

                    if (t.is_error()) {
                        report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                        return false;
                    }

                    if (t.type == TokenType::ClosingSquareBracket) {
                        break;
                    }

                    if (t.type != TokenType::Comma) {
                        report_error(ErrorType::SYNTAX, "expected `,` or `]`, but got `%s`", t.view());
                        return false;
                    }

                    lexer.consume();
                    lexer.consume_white_space(true);
                    t = lexer.look_at();

                    if (t.is_error()) {
                        report_error(ErrorType::SYNTAX, "%s", get_error_token_description(t.type));
                        return false;
                    }
                }
            }

            lexer.consume();
        } else {
            if (!parse_one(lexer)) {
                return false;
            }
        }

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

        // Parse trailing comments.
        tailing_comment = pool->create_node<CommentNode>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            return false;
        }

        const std::string shown = format_payload(payload);
        report_debug(ErrorType::NONE, "annotation `%s` classify `%s` value size `%zu` value `%s`", key, classify, payload.size(), shown.c_str());
        return true;
    }

    namespace {

        bool is_key_type(const StructNode* node) {
            if (node == nullptr || !node->builtin || node->generic) {
                return false;
            }
            switch (node->phy_type) {
            case PhysicalType::BOOL:
            case PhysicalType::INT8:
            case PhysicalType::UINT8:
            case PhysicalType::INT16:
            case PhysicalType::UINT16:
            case PhysicalType::INT32:
            case PhysicalType::UINT32:
            case PhysicalType::INT64:
            case PhysicalType::UINT64:
            case PhysicalType::STRING:
                return true;
            default:
                return false;
            }
        }

        bool integer_in_width(PhysicalType type, int64_t number) {
            switch (type) {
            case PhysicalType::INT8:
                return number >= std::numeric_limits<int8_t>::min() && number <= std::numeric_limits<int8_t>::max();
            case PhysicalType::UINT8:
                return number >= 0 && number <= std::numeric_limits<uint8_t>::max();
            case PhysicalType::INT16:
                return number >= std::numeric_limits<int16_t>::min() && number <= std::numeric_limits<int16_t>::max();
            case PhysicalType::UINT16:
                return number >= 0 && number <= std::numeric_limits<uint16_t>::max();
            case PhysicalType::INT32:
                return number >= std::numeric_limits<int32_t>::min() && number <= std::numeric_limits<int32_t>::max();
            case PhysicalType::UINT32:
                return number >= 0 && number <= std::numeric_limits<uint32_t>::max();
            case PhysicalType::INT64:
                return true;
            case PhysicalType::UINT64:
                return number >= 0;
            default:
                return false;
            }
        }

        bool enum_in_width(EnumNode::UnderlyingType type, int64_t number) {
            switch (type) {
            case EnumNode::UnderlyingType::INT8:
                return number >= std::numeric_limits<int8_t>::min() && number <= std::numeric_limits<int8_t>::max();
            case EnumNode::UnderlyingType::UINT8:
                return number >= 0 && number <= std::numeric_limits<uint8_t>::max();
            case EnumNode::UnderlyingType::INT16:
                return number >= std::numeric_limits<int16_t>::min() && number <= std::numeric_limits<int16_t>::max();
            case EnumNode::UnderlyingType::UINT16:
                return number >= 0 && number <= std::numeric_limits<uint16_t>::max();
            case EnumNode::UnderlyingType::NONE:
            case EnumNode::UnderlyingType::INT32:
                return number >= std::numeric_limits<int32_t>::min() && number <= std::numeric_limits<int32_t>::max();
            case EnumNode::UnderlyingType::UINT32:
                return number >= 0 && number <= std::numeric_limits<uint32_t>::max();
            }
            return false;
        }

        bool same_field(const SymbolNode* symbol, std::string_view name) {
            return symbol->name == name;
        }

    }  // namespace

    bool AnnotationNode::bind_typed() {
        if (!typed) {
            return true;
        }
        if (object == nullptr || file == nullptr) {
            report_error(ErrorType::SEMANTIC, "annotation `%s` has no value", key);
            return false;
        }

        std::function<bool(const TypeRef&, const ConstValue*)> check;
        check = [&](const TypeRef& ref, const ConstValue* value) -> bool {
            if (value == nullptr) {
                report_error(ErrorType::SEMANTIC, "annotation `%s` value is missing", key);
                return false;
            }
            SymbolNode* symbol = file->find_type(type_ref_name(ref));
            if (symbol != nullptr) {
                symbol = file->concrete_type(symbol);
            }
            if (symbol == nullptr) {
                report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` was not found", key, format_type_ref(ref).c_str());
                return false;
            }
            if (symbol->node_type == NodeType::ALIAS) {
                report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` was not bound", key, format_type_ref(ref).c_str());
                return false;
            }

            const auto* named = std::get_if<TypeName>(&ref);
            if (symbol->node_type == NodeType::STRUCT) {
                auto* type = static_cast<StructNode*>(symbol);
                if (type->generic) {
                    if (named == nullptr) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires type arguments", key, type->name);
                        return false;
                    }
                    if (type->phy_type == PhysicalType::LIST || type->phy_type == PhysicalType::SET) {
                        if (type->phy_type == PhysicalType::SET) {
                            SymbolNode* element = file->find_type(type_ref_name(named->args[0].value));
                            if (element != nullptr) {
                                element = file->concrete_type(element);
                            }
                            auto* element_type = element != nullptr && element->node_type == NodeType::STRUCT
                                ? static_cast<StructNode*>(element)
                                : nullptr;
                            if (!is_key_type(element_type)) {
                                report_error(ErrorType::SEMANTIC, "annotation `%s` set element `%s` cannot be a key", key, format_type_ref(named->args[0].value).c_str());
                                return false;
                            }
                        }
                        const auto* list = std::get_if<ConstValue::List>(&value->value);
                        if (list == nullptr) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a list", key, format_type_ref(ref).c_str());
                            return false;
                        }
                        for (const ConstValue* element : list->elements) {
                            if (!check(named->args[0].value, element)) {
                                return false;
                            }
                        }
                        return true;
                    }
                    if (type->phy_type == PhysicalType::MAP) {
                        SymbolNode* key_symbol = file->find_type(type_ref_name(named->args[0].value));
                        if (key_symbol != nullptr) {
                            key_symbol = file->concrete_type(key_symbol);
                        }
                        auto* key_type = key_symbol != nullptr && key_symbol->node_type == NodeType::STRUCT
                            ? static_cast<StructNode*>(key_symbol)
                            : nullptr;
                        if (!is_key_type(key_type)) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` map key `%s` cannot be a key", key, format_type_ref(named->args[0].value).c_str());
                            return false;
                        }
                        const auto* map = std::get_if<ConstValue::Map>(&value->value);
                        if (map == nullptr) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a map", key, format_type_ref(ref).c_str());
                            return false;
                        }
                        for (const auto& entry : map->entries) {
                            if (!check(named->args[0].value, entry.first) || !check(named->args[1].value, entry.second)) {
                                return false;
                            }
                        }
                        return true;
                    }
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` cannot be constructed", key, format_type_ref(ref).c_str());
                    return false;
                }
                if (named != nullptr) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` does not take type arguments", key, format_type_ref(ref).c_str());
                    return false;
                }
                if (type->builtin) {
                    switch (type->phy_type) {
                    case PhysicalType::BOOL:
                        if (!std::holds_alternative<bool>(value->value)) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `bool` requires a bool", key);
                            return false;
                        }
                        return true;
                    case PhysicalType::STRING:
                        if (!std::holds_alternative<std::string_view>(value->value)) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `string` requires a string", key);
                            return false;
                        }
                        return true;
                    case PhysicalType::FLOAT32:
                    case PhysicalType::FLOAT64:
                        if (!std::holds_alternative<double>(value->value)) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a float", key, type->name);
                            return false;
                        }
                        return true;
                    case PhysicalType::INT8:
                    case PhysicalType::UINT8:
                    case PhysicalType::INT16:
                    case PhysicalType::UINT16:
                    case PhysicalType::INT32:
                    case PhysicalType::UINT32:
                    case PhysicalType::INT64:
                    case PhysicalType::UINT64:
                        if (!std::holds_alternative<int64_t>(value->value) || !integer_in_width(type->phy_type, std::get<int64_t>(value->value))) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires an integer in range", key, type->name);
                            return false;
                        }
                        return true;
                    default:
                        report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` cannot be constructed", key, type->name);
                        return false;
                    }
                }
                const auto* map = std::get_if<ConstValue::Map>(&value->value);
                if (map == nullptr) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a struct value", key, type->name);
                    return false;
                }
                std::vector<const FieldNode*> seen;
                for (const auto& entry : map->entries) {
                    const auto* id = std::get_if<ConstValue::Identifier>(&entry.first->value);
                    if (id == nullptr) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` field name is not an identifier", key);
                        return false;
                    }
                    const FieldNode* field = nullptr;
                    for (const FieldNode* candidate : type->fields) {
                        if (same_field(candidate, id->name)) {
                            field = candidate;
                            break;
                        }
                    }
                    if (field == nullptr) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` has no field `%s`", key, type->name, id->name);
                        return false;
                    }
                    for (const FieldNode* previous : seen) {
                        if (previous == field) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` field `%s` is repeated", key, field->name);
                            return false;
                        }
                    }
                    seen.push_back(field);
                    if (!check(field->type_ref, entry.second)) {
                        return false;
                    }
                }
                for (const FieldNode* field : type->fields) {
                    bool present = false;
                    for (const FieldNode* previous : seen) {
                        if (previous == field) {
                            present = true;
                            break;
                        }
                    }
                    if (!present && field->default_value == nullptr) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` field `%s` is missing", key, field->name);
                        return false;
                    }
                }
                return true;
            }
            if (symbol->node_type == NodeType::ENUM) {
                if (named != nullptr) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` does not take type arguments", key, format_type_ref(ref).c_str());
                    return false;
                }
                auto* enum_type = static_cast<EnumNode*>(symbol);
                if (const auto* number = std::get_if<int64_t>(&value->value)) {
                    if (!enum_in_width(enum_type->underlying, *number)) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` enum `%s` value is out of range", key, enum_type->name);
                        return false;
                    }
                    return true;
                }
                const auto* id = std::get_if<ConstValue::Identifier>(&value->value);
                if (id == nullptr) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` enum `%s` requires an integer or an enumerator", key, enum_type->name);
                    return false;
                }
                for (const EnumValueNode* item : enum_type->values) {
                    if (item->name == id->name || item->full_name == id->name) {
                        return true;
                    }
                }
                report_error(ErrorType::SEMANTIC, "annotation `%s` enum `%s` has no value `%s`", key, enum_type->name, id->name);
                return false;
            }
            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` was not bound", key, format_type_ref(ref).c_str());
            return false;
        };

        return check(type_ref, object);
    }

} // namespace shark::parser
