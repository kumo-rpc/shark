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

#include <shark/compiler/cpp/enum.h>
#include <shark/common/clog.h>

#include <map>
#include <string>

namespace shark::compiler::cpp {
namespace {

bool global_enum(const SchemaEnum* item) {
    return item != nullptr && item->container() == nullptr && !item->name().empty();
}

std::string escape_printer_text(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == '$') {
            out.append("$$");
        } else {
            out.push_back(c);
        }
    }
    return out;
}

void print_comment(io::Printer& printer, const SchemaComment* comment) {
    if (comment == nullptr) {
        return;
    }
    for (uint32_t line = 0; line < comment->comment_size(); ++line) {
        std::map<std::string, std::string> vars;
        vars["text"] = escape_printer_text(comment->comment(line));
        printer.print(vars, "$text$\n");
    }
}

std::string underlying_type_name(EnumBase::UnderlyingType type) {
    switch (type) {
        case EnumBase::UnderlyingType::INT8:
            return "int8_t";
        case EnumBase::UnderlyingType::UINT8:
            return "uint8_t";
        case EnumBase::UnderlyingType::INT16:
            return "int16_t";
        case EnumBase::UnderlyingType::UINT16:
            return "uint16_t";
        case EnumBase::UnderlyingType::INT32:
            return "int32_t";
        case EnumBase::UnderlyingType::UINT32:
            return "uint32_t";
        case EnumBase::UnderlyingType::NONE:
            return {};
    }
    return {};
}

bool generate_values(io::Printer& printer, const SchemaEnum* item) {
    printer.indent();
    for (uint32_t i = 0; i < item->value_size(); ++i) {
        const SchemaEnumValue* value = item->value(i);
        if (value == nullptr) {
            continue;
        }
        for (uint32_t c = 0; c < value->leading_comment_size(); ++c) {
            print_comment(printer, value->leading_comment(c));
        }
        std::map<std::string, std::string> vars;
        vars["name"] = value->name();
        const SchemaField* field = value->constexpr_field();
        vars["value"] = field == nullptr ? std::to_string(value->value()) : field->name();
        const bool last = i + 1 == item->value_size();
        std::string tail;
        const SchemaComment* tailing = value->tailing_comment();
        if (tailing != nullptr) {
            for (uint32_t line = 0; line < tailing->comment_size(); ++line) {
                if (!tail.empty()) {
                    tail.push_back(' ');
                }
                tail += escape_printer_text(tailing->comment(line));
            }
        }
        if (tail.empty()) {
            printer.print(vars, last ? "$name$ = $value$\n" : "$name$ = $value$,\n");
        } else {
            vars["tail"] = tail;
            printer.print(vars, last ? "$name$ = $value$ $tail$\n" : "$name$ = $value$, $tail$\n");
        }
    }
    printer.outdent();
    return true;
}

}  // namespace

void EnumGenerator::generate_global_declare(io::Printer& printer) {
    if (!global_enum(_enum)) {
        return;
    }
    for (uint32_t i = 0; i < _enum->leading_comment_size(); ++i) {
        print_comment(printer, _enum->leading_comment(i));
    }
    std::map<std::string, std::string> vars;
    vars["name"] = _enum->name();
    if (_enum->is_class()) {
        const std::string underlying = underlying_type_name(_enum->underlying());
        if (underlying.empty()) {
            shark::common::report_error(_enum->full_name() + " enum class underlying type is missing");
            return;
        }
        vars["underlying"] = underlying;
        printer.print(vars, "enum class $name$ : $underlying$ {\n");
    } else {
        printer.print(vars, "enum $name$ {\n");
    }
    if (!generate_values(printer, _enum)) {
        printer.print("};\n");
        return;
    }
    printer.print("};\n");
    print_comment(printer, _enum->tailing_comment());
}

void EnumGenerator::generate_nested_declare(io::Printer& printer) {}

void EnumGenerator::generate_inline_define(io::Printer& printer) {}

void EnumGenerator::generate_source_define(io::Printer& printer) {}

}  // namespace shark::compiler::cpp
