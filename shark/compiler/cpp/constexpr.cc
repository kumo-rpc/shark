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

#include <shark/compiler/cpp/constexpr.h>
#include <shark/common/clog.h>
#include <shark/schema/constrain.h>

#include <map>
#include <string>

namespace shark::compiler::cpp {
namespace {

bool primitive_const(const SchemaField* field) {
    if (field == nullptr) {
        return false;
    }
    return type_is_primitive(field->type());
}

std::string cpp_type_name(const SchemaType* type) {
    if (type == nullptr) {
        return {};
    }
    for (uint32_t i = 0; i < type->annotation_size(); ++i) {
        const SchemaAnnotation* annotation = type->annotation(i);
        if (annotation == nullptr || annotation->classify() != "cpp" || annotation->key() != "type") {
            continue;
        }
        const SchemaConstValue* value = annotation->value();
        if (value == nullptr || value->kind() != SchemaConstValue::Kind::STRING) {
            continue;
        }
        return value->string_value();
    }
    return {};
}

std::string escape_cpp_string(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == '\\' || c == '"') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

std::string const_literal(const SchemaConstValue* value) {
    if (value == nullptr) {
        return {};
    }
    switch (value->kind()) {
        case SchemaConstValue::Kind::BOOL:
            return value->bool_value() ? "true" : "false";
        case SchemaConstValue::Kind::INT:
            return std::to_string(value->int_value());
        case SchemaConstValue::Kind::FLOAT:
            return std::to_string(value->float_value());
        case SchemaConstValue::Kind::STRING:
            return "\"" + escape_cpp_string(value->string_value()) + "\"";
        default:
            return {};
    }
}

}  // namespace

void ConstexprGenerator::generate_primitive_constexpr_declare(io::Printer& printer) {
    const SchemaField* field = _field;
    if (!primitive_const(field)) {
        return;
    }
    const std::string type_name = cpp_type_name(field->type());
    const std::string literal = const_literal(field->default_value());
    if (type_name.empty() || literal.empty()) {
        shark::common::report_error(field->full_name() + " constexpr value is missing");
        return;
    }
    std::map<std::string, std::string> vars;
    vars["type"] = type_name;
    vars["name"] = field->name();
    vars["value"] = literal;
    if (type_is_constexpr(field->type())) {
        printer.print(vars, "static constexpr $type$ $name$ = $value$;\n");
        return;
    }
    printer.print(vars, "extern const $type$ $name$;\n");
}

void ConstexprGenerator::generate_primitive_constexpr_define(io::Printer& printer) {
    const SchemaField* field = _field;
    if (!primitive_const(field) || type_is_constexpr(field->type())) {
        return;
    }
    const std::string type_name = cpp_type_name(field->type());
    const std::string literal = const_literal(field->default_value());
    if (type_name.empty() || literal.empty()) {
        shark::common::report_error(field->full_name() + " constexpr value is missing");
        return;
    }
    std::map<std::string, std::string> vars;
    vars["type"] = type_name;
    vars["name"] = field->name();
    vars["value"] = literal;
    printer.print(vars, "const $type$ $name$ = $value$;\n");
}

void ConstexprGenerator::generate_enum_constexpr_declare(io::Printer&) {}

void ConstexprGenerator::generate_enum_constexpr_define(io::Printer&) {}

void ConstexprGenerator::generate_struct_constexpr_declare(io::Printer&) {}

void ConstexprGenerator::generate_struct_constexpr_define(io::Printer&) {}

}  // namespace shark::compiler::cpp
