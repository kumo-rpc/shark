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

#include <shark/common/clog.h>
#include <shark/compiler/cpp/enum.h>

#include <map>
#include <string>

#include <turbo/strings/ascii.h>
#include <turbo/strings/str_replace.h>

namespace shark::compiler::cpp {
    namespace {

        bool global_enum(const SchemaEnum* item) {
            return item != nullptr && item->container() == nullptr && !item->name().empty();
        }

        std::string ascii_lower(const std::string& text) {
            std::string out(text.size(), '\0');
            for (std::size_t i = 0; i < text.size(); ++i) {
                out[i] = turbo::ascii_tolower(static_cast<unsigned char>(text[i]));
            }
            return out;
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
                return { };
            }
            return { };
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

    } // namespace

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

    void EnumGenerator::generate_traits_declare(io::Printer& printer) {
        if (!global_enum(_enum)) {
            return;
        }
        std::string type = turbo::StrReplaceAll(_enum->full_name(), {{".", "::"}});
        std::string underlying = "int32_t";
        if (_enum->is_class()) {
            underlying = underlying_type_name(_enum->underlying());
            if (underlying.empty()) {
                shark::common::report_error(_enum->full_name() + " enum class underlying type is missing");
                return;
            }
        }
        bool has_value = false;
        int64_t min_value = 0;
        int64_t max_value = 0;
        for (uint32_t i = 0; i < _enum->value_size(); ++i) {
            const SchemaEnumValue* value = _enum->value(i);
            if (value == nullptr) {
                continue;
            }
            if (!has_value) {
                min_value = value->value();
                max_value = value->value();
                has_value = true;
                continue;
            }
            if (value->value() < min_value) {
                min_value = value->value();
            }
            if (value->value() > max_value) {
                max_value = value->value();
            }
        }
        std::map<std::string, std::string> vars;
        vars["type"] = type;
        vars["underlying"] = underlying;
        vars["min"] = std::to_string(min_value);
        vars["max"] = std::to_string(max_value);
        vars["count"] = std::to_string(_enum->value_size());
        printer.print(vars,
            "template <>\n"
            "struct EnumTraitImpl<$type$> {\n"
            "    using underlying = $underlying$;\n"
            "    static constexpr $type$ enum_min = static_cast<$type$>($min$);\n"
            "    static constexpr $type$ enum_max = static_cast<$type$>($max$);\n"
            "    static constexpr uint32_t value_count = $count$;\n"
            "    static constexpr bool has_schema = true;\n"
            "    static const SchemaEnum* schema();\n"
            "    static std::string_view enum_name($type$ enum_t_value);\n"
            "    static bool enum_parse(std::string_view name, $type$* value);\n"
            "    static bool is_valid(int64_t value);\n"
            "    static fermat::flat_hash_map<std::string_view, $type$> name_map;\n"
            "    static fermat::flat_hash_map<std::string_view, $type$> lower_name_map;\n"
            "};\n"
            "\n"
            "template <>\n"
            "struct IsSharkEnum<$type$> : ::std::true_type {};\n");
    }

    void EnumGenerator::generate_traits_define(io::Printer& printer) {
        if (!global_enum(_enum)) {
            return;
        }
        std::string type = turbo::StrReplaceAll(_enum->full_name(), {{".", "::"}});
        std::map<std::string, std::string> vars;
        vars["type"] = type;
        printer.print(vars,
            "const SchemaEnum* EnumTraitImpl<$type$>::schema() {\n"
            "    return nullptr;\n"
            "}\n"
            "\n"
            "std::string_view EnumTraitImpl<$type$>::enum_name($type$ enum_t_value) {\n"
            "    switch (enum_t_value) {\n");
        std::map<int64_t, bool> seen;
        for (uint32_t i = 0; i < _enum->value_size(); ++i) {
            const SchemaEnumValue* value = _enum->value(i);
            if (value == nullptr || !seen.emplace(value->value(), true).second) {
                continue;
            }
            vars["case"] = turbo::StrReplaceAll(value->full_name(), {{".", "::"}});
            vars["name"] = escape_printer_text(value->name());
            printer.print(vars,
                "        case $case$:\n"
                "            return \"$name$\";\n");
        }
        printer.print(vars,
            "        default:\n"
            "            return {};\n"
            "    }\n"
            "}\n"
            "\n"
            "bool EnumTraitImpl<$type$>::is_valid(int64_t value) {\n"
            "    switch (value) {\n");
        seen.clear();
        for (uint32_t i = 0; i < _enum->value_size(); ++i) {
            const SchemaEnumValue* value = _enum->value(i);
            if (value == nullptr || !seen.emplace(value->value(), true).second) {
                continue;
            }
            vars["number"] = std::to_string(value->value());
            printer.print(vars,
                "        case $number$:\n"
                "            return true;\n");
        }
        printer.print(vars,
            "        default:\n"
            "            return false;\n"
            "    }\n"
            "}\n"
            "\n"
            "fermat::flat_hash_map<std::string_view, $type$> EnumTraitImpl<$type$>::name_map{\n");
        for (uint32_t i = 0; i < _enum->value_size(); ++i) {
            const SchemaEnumValue* value = _enum->value(i);
            if (value == nullptr) {
                continue;
            }
            vars["name"] = escape_printer_text(value->name());
            vars["case"] = turbo::StrReplaceAll(value->full_name(), {{".", "::"}});
            const bool last = i + 1 == _enum->value_size();
            printer.print(vars, last ? "    {\"$name$\", $case$}\n" : "    {\"$name$\", $case$},\n");
        }
        printer.print("};\n\n");
        printer.print(vars, "fermat::flat_hash_map<std::string_view, $type$> EnumTraitImpl<$type$>::lower_name_map{\n");
        for (uint32_t i = 0; i < _enum->value_size(); ++i) {
            const SchemaEnumValue* value = _enum->value(i);
            if (value == nullptr) {
                continue;
            }
            vars["name"] = escape_printer_text(ascii_lower(value->name()));
            vars["case"] = turbo::StrReplaceAll(value->full_name(), {{".", "::"}});
            const bool last = i + 1 == _enum->value_size();
            printer.print(vars, last ? "    {\"$name$\", $case$}\n" : "    {\"$name$\", $case$},\n");
        }
        printer.print("};\n\n");
        printer.print(vars,
            "bool EnumTraitImpl<$type$>::enum_parse(std::string_view name, $type$* value) {\n"
            "    auto it = name_map.find(name);\n"
            "    if (it == name_map.end()) {\n"
            "        return false;\n"
            "    }\n"
            "    *value = it->second;\n"
            "    return true;\n"
            "}\n");
    }

    void EnumGenerator::generate_nested_declare(io::Printer& printer) { }

    void EnumGenerator::generate_inline_define(io::Printer& printer) { }

    void EnumGenerator::generate_source_define(io::Printer& printer) { }

} // namespace shark::compiler::cpp
