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

#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace shark::parser {

    namespace {

        bool unsupported_width(const TypeName& ref) {
            if (ref.name == "int128" || ref.name == "uint128") {
                return true;
            }
            for (const TypeName& arg : ref.arguments) {
                if (unsupported_width(arg)) {
                    return true;
                }
            }
            return false;
        }

        void append_const(std::string& out, const ConstValueParser* value) {
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
            } else if (const auto* id = std::get_if<ConstValueParser::Identifier>(&value->value)) {
                out.append(id->name.data(), id->name.size());
            } else if (const auto* list = std::get_if<ConstValueParser::List>(&value->value)) {
                out.push_back('[');
                for (std::size_t i = 0; i < list->elements.size(); ++i) {
                    if (i != 0) {
                        out.append(", ");
                    }
                    append_const(out, list->elements[i]);
                }
                out.push_back(']');
            } else if (const auto* map = std::get_if<ConstValueParser::Map>(&value->value)) {
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
            } else if (const auto* fields = std::get_if<ConstValueParser::Struct>(&value->value)) {
                out.push_back('{');
                for (std::size_t i = 0; i < fields->fields.size(); ++i) {
                    if (i != 0) {
                        out.append(", ");
                    }
                    if (fields->fields[i].field != nullptr) {
                        out.append(fields->fields[i].field->name.data(), fields->fields[i].field->name.size());
                    }
                    out.append(" = ");
                    append_const(out, fields->fields[i].value);
                }
                out.push_back('}');
            }
        }

        std::size_t const_size(const ConstValueParser* value) {
            if (value == nullptr) {
                return 0;
            }
            if (const auto* list = std::get_if<ConstValueParser::List>(&value->value)) {
                return list->elements.size();
            }
            if (const auto* map = std::get_if<ConstValueParser::Map>(&value->value)) {
                return map->entries.size();
            }
            if (const auto* fields = std::get_if<ConstValueParser::Struct>(&value->value)) {
                return fields->fields.size();
            }
            return 1;
        }

        bool is_key_type(const StructParser* node) {
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

        bool enum_in_width(EnumParser::UnderlyingType type, int64_t number) {
            switch (type) {
            case EnumParser::UnderlyingType::INT8:
                return number >= std::numeric_limits<int8_t>::min() && number <= std::numeric_limits<int8_t>::max();
            case EnumParser::UnderlyingType::UINT8:
                return number >= 0 && number <= std::numeric_limits<uint8_t>::max();
            case EnumParser::UnderlyingType::INT16:
                return number >= std::numeric_limits<int16_t>::min() && number <= std::numeric_limits<int16_t>::max();
            case EnumParser::UnderlyingType::UINT16:
                return number >= 0 && number <= std::numeric_limits<uint16_t>::max();
            case EnumParser::UnderlyingType::NONE:
            case EnumParser::UnderlyingType::INT32:
                return number >= std::numeric_limits<int32_t>::min() && number <= std::numeric_limits<int32_t>::max();
            case EnumParser::UnderlyingType::UINT32:
                return number >= 0 && number <= std::numeric_limits<uint32_t>::max();
            }
            return false;
        }

        bool same_field(const SymbolNode* symbol, std::string_view name) {
            return symbol->name == name;
        }

    }  // namespace

    bool AnnotationParser::parse_typed_value(Lexer<ExpressionLexer>& lexer) {
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
        object = pool->create_node<ConstValueParser>(file);
        return object->parse(lexer);
    }

    bool AnnotationParser::parse(Lexer<ExpressionLexer>& lexer) {
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

        classify = {};

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
        } else if (t.type == TokenType::Equals) {
            lexer.consume();
            object = pool->create_node<ConstValueParser>(file);
            if (!object->parse(lexer)) {
                return false;
            }
        } else {
            report_error(ErrorType::SYNTAX, "expected `=` or `<`, but got `%s`", t.view());
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

        tailing_comment = pool->create_node<CommentParser>(file);
        if (!tailing_comment->parse_tailing(lexer)) {
            return false;
        }
        std::string shown;
        append_const(shown, object);
        report_debug(ErrorType::NONE, "annotation `%s` classify `%s` type `%s` value size `%zu` value `%s`", key, classify, format_type_ref(type_ref).c_str(), const_size(object), shown.c_str());
        return true;
    }

    bool AnnotationParser::bind_typed() {
        if (!typed) {
            return true;
        }
        if (object == nullptr || file == nullptr) {
            report_error(ErrorType::SEMANTIC, "annotation `%s` has no value", key);
            return false;
        }

        std::function<bool(const TypeName&, ConstValueParser*)> check;
        check = [&](const TypeName& ref, ConstValueParser* value) -> bool {
            if (value == nullptr) {
                report_error(ErrorType::SEMANTIC, "annotation `%s` value is missing", key);
                return false;
            }
            SymbolNode* symbol = file->find_type(ref);
            if (symbol != nullptr) {
                symbol = file->concrete_type(symbol);
            }
            if (symbol == nullptr) {
                report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` was not found", key, format_type_ref(ref).c_str());
                return false;
            }
            if (symbol->node_type == SymbolType::ALIAS) {
                report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` was not bound", key, format_type_ref(ref).c_str());
                return false;
            }

            const bool has_arguments = !ref.arguments.empty();
            if (symbol->node_type == SymbolType::STRUCT) {
                auto* type = static_cast<StructParser*>(symbol);
                if (type->generic) {
                    if (!has_arguments) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires type arguments", key, type->name);
                        return false;
                    }
                    if (type->phy_type == PhysicalType::LIST || type->phy_type == PhysicalType::SET) {
                        if (type->phy_type == PhysicalType::SET) {
                            SymbolNode* element = file->find_type(ref.arguments[0]);
                            if (element != nullptr) {
                                element = file->concrete_type(element);
                            }
                            auto* element_type = element != nullptr && element->node_type == SymbolType::STRUCT
                                ? static_cast<StructParser*>(element)
                                : nullptr;
                            if (!is_key_type(element_type)) {
                                report_error(ErrorType::SEMANTIC, "annotation `%s` set element `%s` cannot be a key", key, format_type_ref(ref.arguments[0]).c_str());
                                return false;
                            }
                        }
                        const auto* list = std::get_if<ConstValueParser::List>(&value->value);
                        if (list == nullptr) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a list", key, format_type_ref(ref).c_str());
                            return false;
                        }
                        for (ConstValueParser* element : list->elements) {
                            if (!check(ref.arguments[0], element)) {
                                return false;
                            }
                        }
                        return true;
                    }
                    if (type->phy_type == PhysicalType::MAP) {
                        SymbolNode* key_symbol = file->find_type(ref.arguments[0]);
                        if (key_symbol != nullptr) {
                            key_symbol = file->concrete_type(key_symbol);
                        }
                        auto* key_type = key_symbol != nullptr && key_symbol->node_type == SymbolType::STRUCT
                            ? static_cast<StructParser*>(key_symbol)
                            : nullptr;
                        if (!is_key_type(key_type)) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` map key `%s` cannot be a key", key, format_type_ref(ref.arguments[0]).c_str());
                            return false;
                        }
                        const auto* map = std::get_if<ConstValueParser::Map>(&value->value);
                        if (map == nullptr) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a map", key, format_type_ref(ref).c_str());
                            return false;
                        }
                        for (const auto& entry : map->entries) {
                            if (!check(ref.arguments[0], entry.first) || !check(ref.arguments[1], entry.second)) {
                                return false;
                            }
                        }
                        return true;
                    }
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` cannot be constructed", key, format_type_ref(ref).c_str());
                    return false;
                }
                if (has_arguments) {
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
                const auto* map = std::get_if<ConstValueParser::Map>(&value->value);
                if (map == nullptr) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` requires a struct value", key, type->name);
                    return false;
                }
                std::vector<std::pair<const FieldParser*, ConstValueParser*>> seen;
                for (const auto& entry : map->entries) {
                    const auto* id = std::get_if<ConstValueParser::Identifier>(&entry.first->value);
                    if (id == nullptr) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` field name is not an identifier", key);
                        return false;
                    }
                    const FieldParser* field = nullptr;
                    for (const FieldParser* candidate : type->fields) {
                        if (same_field(candidate, id->name)) {
                            field = candidate;
                            break;
                        }
                    }
                    if (field == nullptr) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` has no field `%s`", key, type->name, id->name);
                        return false;
                    }
                    for (const auto& previous : seen) {
                        if (previous.first == field) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` field `%s` is repeated", key, field->name);
                            return false;
                        }
                    }
                    if (!check(field->type_ref, entry.second)) {
                        return false;
                    }
                    seen.push_back({field, entry.second});
                }
                ConstValueParser::Struct built;
                for (const FieldParser* field : type->fields) {
                    ConstValueParser* present = nullptr;
                    for (const auto& previous : seen) {
                        if (previous.first == field) {
                            present = previous.second;
                            break;
                        }
                    }
                    if (present == nullptr) {
                        if (field->default_value == nullptr) {
                            report_error(ErrorType::SEMANTIC, "annotation `%s` field `%s` is missing", key, field->name);
                            return false;
                        }
                        if (!check(field->type_ref, field->default_value)) {
                            return false;
                        }
                        built.fields.push_back(ConstValueParser::StructField{field, field->default_value});
                        continue;
                    }
                    built.fields.push_back(ConstValueParser::StructField{field, present});
                }
                value->value = std::move(built);
                return true;
            }
            if (symbol->node_type == SymbolType::ENUM) {
                if (has_arguments) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` type `%s` does not take type arguments", key, format_type_ref(ref).c_str());
                    return false;
                }
                auto* enum_type = static_cast<EnumParser*>(symbol);
                if (const auto* number = std::get_if<int64_t>(&value->value)) {
                    if (!enum_in_width(enum_type->underlying, *number)) {
                        report_error(ErrorType::SEMANTIC, "annotation `%s` enum `%s` value is out of range", key, enum_type->name);
                        return false;
                    }
                    for (const EnumValueParser* item : enum_type->values) {
                        if (item->value == *number) {
                            return true;
                        }
                    }
                    return true;
                }
                auto* id = std::get_if<ConstValueParser::Identifier>(&value->value);
                if (id == nullptr) {
                    report_error(ErrorType::SEMANTIC, "annotation `%s` enum `%s` requires an integer or an enumerator", key, enum_type->name);
                    return false;
                }
                for (const EnumValueParser* item : enum_type->values) {
                    if (item->name == id->name || item->full_name == id->name) {
                        id->enum_type = enum_type;
                        id->enumerator = item;
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
