// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <shark/compiler/cpp/enum.h>

#include <cstdint>
#include <limits>
#include <map>

#include <shark/compiler/cpp/helpers.h>
#include <shark/compiler/cpp/names.h>
#include <shark/io/printer.h>
#include <shark/stubs/strutil.h>

namespace shark::compiler::cpp {

    namespace {
        // The GOOGLE_ARRAYSIZE constant is the max enum value plus 1. If the max enum value
        // is kint32max, GOOGLE_ARRAYSIZE will overflow. In such cases we should omit the
        // generation of the GOOGLE_ARRAYSIZE constant.
        bool ShouldGenerateArraySize(const EnumDescriptor* descriptor) {
            int32_t max_value = descriptor->value(0)->number();
            for (int i = 0; i < descriptor->value_count(); i++) {
                if (descriptor->value(i)->number() > max_value) {
                    max_value = descriptor->value(i)->number();
                }
            }
            return max_value != std::numeric_limits<int32_t>::max();
        }

        // Returns the number of unique numeric enum values. This is less than
        // descriptor->value_count() when there are aliased values.
        int CountUniqueValues(const EnumDescriptor* descriptor) {
            std::set<int> values;
            for (int i = 0; i < descriptor->value_count(); ++i) {
                values.insert(descriptor->value(i)->number());
            }
            return values.size();
        }

    } // namespace

    EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor,
        const std::map<std::string, std::string>& vars,
        const Options& options)
        : descriptor_(descriptor)
        , classname_(ClassName(descriptor, false))
        , options_(options)
        , generate_array_size_(ShouldGenerateArraySize(descriptor))
        , variables_(vars) {
        variables_["classname"] = classname_;
        variables_["classtype"] = QualifiedClassName(descriptor_, options);
        variables_["short_name"] = descriptor_->name();
        variables_["nested_name"] = descriptor_->name();
        variables_["resolved_name"] = ResolveKeyword(descriptor_->name());
        variables_["prefix"] = (descriptor_->containing_type() == nullptr) ? "" : classname_ + "_";
        variables_["underlying"] = EnumUnderlyingCppType(descriptor_);
        if (descriptor_->containing_type()) {
            inner_ = true;
        }
    }

    EnumGenerator::~EnumGenerator() { }

    namespace {

        std::string FitsIntExpression(const EnumDescriptor* descriptor) {
            switch (descriptor->options().underlying_type()) {
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT8:
                return "value >= -128 && value <= 127";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT8:
                return "value >= 0 && value <= 255";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT16:
                return "value >= -32768 && value <= 32767";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT16:
                return "value >= 0 && value <= 65535";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT32:
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT32:
                return "true";
            default:
                return "true";
            }
        }

        std::string fits_wire_expression(const EnumDescriptor* descriptor,
            const std::string& classname) {
            switch (descriptor->options().underlying_type()) {
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT8:
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT16:
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_INT32:
                return "(value <= 2147483647u || (value >> 32) == 0xffffffffu) && fits_underlying(static_cast<int64_t>(value))";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT8:
                return "value <= 255u";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT16:
                return "value <= 65535u";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UINT32:
                return "value <= 0xffffffffu";
            case EnumUnderlyingType::ENUM_UNDERLYING_TYPE_UNSPECIFIED:
            default:
                return "(value <= 2147483647u || (value >> 32) == 0xffffffffu) && fits_underlying(static_cast<int64_t>(value))";
            }
        }

    } // namespace

    void EnumGenerator::GenerateDefinition(io::Printer* printer, bool inner) {
        Formatter format(printer, variables_);
        // "enum" and "enum class" are equivalent: always generate a C++ enum
        // class with the selected underlying type.
        if (inner_) {
            format("// inner enum $resolved_name$\n");
        } else {
            format("// global enum $resolved_name$\n");
        }
        format("// enum class $classname$ : $underlying$ {\n");
        format("enum class ${1$$classname$$}$ : $underlying$ {\n", descriptor_);
        format.Indent();

        min_value_ = descriptor_->value(0);
        max_value_ = descriptor_->value(0);

        for (int i = 0; i < descriptor_->value_count(); i++) {
            auto format_value = format;
            format_value.

                Set("name", EnumValueName(descriptor_->value(i)));
            // In C++, an value of -2147483648 gets interpreted as the negative of
            // 2147483648, and since 2147483648 can't fit in an integer, this produces a
            // compiler warning.  This works around that issue.
            format_value.Set("number", Int32ToString(descriptor_->value(i)->number()));
            format_value.Set("deprecation",
                DeprecatedAttribute(options_, descriptor_->value(i)));

            if (i > 0)
                format_value(",\n");
            format_value("${1$$name$$}$ $deprecation$= $number$",
                descriptor_->value(i));

            if (descriptor_->value(i)->number() < min_value_->number()) {
                min_value_ = descriptor_->value(i);
            }
            if (descriptor_->value(i)->number() > max_value_->number()) {
                max_value_ = descriptor_->value(i);
            }
        }

        format.Outdent();
        format("\n};\n");
        format("////////////////////////////////////////////////////////////////////\n");
        format("/// enum traits\n");

        {
            bool has_des = HasDescriptorMethods(descriptor_->file(), options_);
            format("struct $classname$Trait {\n");
            format.Indent();
            format("using underlying = $underlying$;\n");
            format("static constexpr $classname$ enum_min = $classname$::$1$;\n", EnumValueName(min_value_));
            format("static constexpr $classname$ enum_max = $classname$::$1$;\n", EnumValueName(max_value_));
            format("static constexpr uint32_t value_count = $1$;\n", descriptor_->value_count());
            format("static constexpr bool has_descriptor = $1$;\n", (has_des ? "true" : "false"));

            if (has_des) {
                format("static const ::$proto_ns$::EnumDescriptor* descriptor();\n");
            }

            format("static $underlying$ to_underlying($classname$ value) { return static_cast<$underlying$>(value); }\n");

            format("static bool is_valid(int64_t value);\n");

            format("static const std::string& enum_name($classname$ e);\n");

            format("static bool enum_parse(::SHARK_NAMESPACE_ID::ConstStringParam name, $classname$ *e);\n");

            format("\n\n");

            format(
                "static constexpr bool fits_underlying(int64_t value) {\n"
                "     return $1$;\n"
                "}\n"
                "static constexpr bool fits_wire(::uint64_t value) {\n"
                "     return $2$;\n"
                "}\n",
                FitsIntExpression(descriptor_),
                fits_wire_expression(descriptor_, classname_));

            format.Outdent();
            format("};\n\n");
        }

        {
            if (descriptor_->options().enable_operator()) {
                format("////////////////////////////////////////////////////////////////////\n");
                format("/// enum operators\n");
                format(
                    "constexpr $classname$ operator|($classname$ lhs, $classname$ rhs) {\n"
                    "  return static_cast<$classname$>(static_cast<$underlying$>("
                    "static_cast<$underlying$>(lhs) | static_cast<$underlying$>(rhs)));\n"
                    "}\n"
                    "constexpr $classname$ operator&($classname$ lhs, $classname$ rhs) {\n"
                    "  return static_cast<$classname$>(static_cast<$underlying$>("
                    "static_cast<$underlying$>(lhs) & static_cast<$underlying$>(rhs)));\n"
                    "}\n"
                    "constexpr $classname$ operator^($classname$ lhs, $classname$ rhs) {\n"
                    "  return static_cast<$classname$>(static_cast<$underlying$>("
                    "static_cast<$underlying$>(lhs) ^ static_cast<$underlying$>(rhs)));\n"
                    "}\n"
                    "constexpr $classname$ operator~($classname$ value) {\n"
                    "  return static_cast<$classname$>(static_cast<$underlying$>("
                    "~static_cast<$underlying$>(value)));\n"
                    "}\n"
                    "constexpr $classname$& operator|=($classname$& lhs, $classname$ rhs) {\n"
                    "  return lhs = lhs | rhs;\n"
                    "}\n"
                    "constexpr $classname$& operator&=($classname$& lhs, $classname$ rhs) {\n"
                    "  return lhs = lhs & rhs;\n"
                    "}\n"
                    "constexpr $classname$& operator^=($classname$& lhs, $classname$ rhs) {\n"
                    "  return lhs = lhs ^ rhs;\n"
                    "}\n");
            }
        }
    }

    void EnumGenerator::GenerateGetEnumDescriptorSpecializations(
        io::Printer* printer) {

        bool has_des = HasDescriptorMethods(descriptor_->file(), options_);
        Formatter format(printer, variables_);
        format.Indent();
        format("////////////////////////////////////////////////////////////\n");
        format("/// enum $classtype$\n");
        format("template <>\n");
        format("struct EnumTraitImpl<$classtype$> : public $classtype$Trait {};\n");
        format("template <> struct IsSharkEnum< $classtype$> : ::std::true_type {};\n\n");

        format.Outdent();
    }

    void EnumGenerator::GenerateSymbolImports(io::Printer* printer) const {
        Formatter format(printer, variables_);
        format("using $resolved_name$ = $classname$;\n");
        format("using $resolved_name$Trait = $classname$Trait;\n");
    }

    void EnumGenerator::GenerateMethods(int idx, io::Printer* printer) {
        Formatter format(printer, variables_);
        if (HasDescriptorMethods(descriptor_->file(), options_)) {

            format("const ::$proto_ns$::EnumDescriptor* $classtype$Trait::descriptor() {\n"
                   "    ::$proto_ns$::internal::AssignDescriptors(&$desc_table$);\n"
                   "    return $file_level_enum_descriptors$[$1$];\n"
                   "}\n",
                idx);
        }

        format(
            "bool $classtype$Trait::is_valid(int64_t value) {\n"
            "  switch (value) {\n");

        // Multiple values may have the same number.  Make sure we only cover
        // each number once by first constructing a set containing all valid
        // numbers, then printing a case statement for each element.

        std::set<int> numbers;
        for (int j = 0; j < descriptor_->value_count(); j++) {
            const EnumValueDescriptor* value = descriptor_->value(j);
            numbers.insert(value->number());
        }

        for (std::set<int>::iterator iter = numbers.begin(); iter != numbers.end();
            ++iter) {
            format("    case $1$:\n", Int32ToString(*iter));
        }

        format(
            "      return true;\n"
            "    default:\n"
            "      return false;\n"
            "  }\n"
            "}\n"
            "\n");

        ////////////////////////////////////////////////
        ///
        if (!HasDescriptorMethods(descriptor_->file(), options_)) {
            // In lite mode (where descriptors are unavailable), we generate separate
            // tables for mapping between enum names and numbers. The _entries table
            // contains the bulk of the data and is sorted by name, while
            // _entries_by_number is sorted by number and just contains pointers into
            // _entries. The two tables allow mapping from name to number and number to
            // name, both in time logarithmic in the number of enum entries. This could
            // probably be made faster, but for now the tables are intended to be simple
            // and compact.
            //
            // Enums with allow_alias = true support multiple entries with the same
            // numerical value. In cases where there are multiple names for the same
            // number, we treat the first name appearing in the .shark file as the
            // canonical one.
            std::map<std::string, int> name_to_number;
            std::map<int, std::string> number_to_canonical_name;
            for (int i = 0; i < descriptor_->value_count(); i++) {
                const EnumValueDescriptor* value = descriptor_->value(i);
                name_to_number.emplace(value->name(), value->number());
                // The same number may appear with multiple names, so we use emplace() to
                // let the first name win.
                number_to_canonical_name.emplace(value->number(), value->name());
            }

            format(
                "static ::$proto_ns$::internal::ExplicitlyConstructed<std::string> "
                "$classname$_strings[$1$] = {};\n\n",
                CountUniqueValues(descriptor_));

            // We concatenate all the names for a given enum into one big string
            // literal. If instead we store an array of string literals, the linker
            // seems to put all enum strings for a given .shark file in the same
            // section, which hinders its ability to strip out unused strings.
            format("static const char $classname$_names[] =");
            for (const auto& p : name_to_number) {
                format("\n  \"$1$\"", p.first);
            }
            format(";\n\n");

            format(
                "static const ::$proto_ns$::internal::EnumEntry $classname$_entries[] "
                "= {\n");
            int i = 0;
            std::map<int, int> number_to_index;
            int data_index = 0;
            for (const auto& p : name_to_number) {
                format("  { {$classname$_names + $1$, $2$}, $3$ },\n", data_index,
                    p.first.size(), p.second);
                if (number_to_canonical_name[p.second] == p.first) {
                    number_to_index.emplace(p.second, i);
                }
                ++i;
                data_index += p.first.size();
            }

            format(
                "};\n"
                "\n"
                "static const int $classname$_entries_by_number[] = {\n");
            for (const auto& p : number_to_index) {
                format("  $1$, // $2$ -> $3$\n", p.second, p.first,
                    number_to_canonical_name[p.first]);
            }
            format(
                "};\n"
                "\n");
        }

        ////////////////////////////////////////////////
        ///
        if (HasDescriptorMethods(descriptor_->file(), options_)) {
            format("const std::string& $classtype$Trait::enum_name(\n"
                   "   $classtype$ enum_t_value) {\n"
                   "  return ::$proto_ns$::internal::NameOfEnum(\n"
                   "    descriptor(),\n"
                   "    static_cast<$underlying$>(enum_t_value));\n");
            format("}\n");
        } else {
            format(
                "const std::string& $classtype$Trait::enum_name(\n"
                "    $classtype$ value) {\n"
                "  static const bool dummy =\n"
                "      ::$proto_ns$::internal::InitializeEnumStrings(\n"
                "          $classname$_entries,\n"
                "          $classname$_entries_by_number,\n"
                "          $1$, $classname$_strings);\n"
                "  (void) dummy;\n"
                "  int idx = ::$proto_ns$::internal::LookUpEnumName(\n"
                "      $classname$_entries,\n"
                "      $classname$_entries_by_number,\n"
                "      $1$, static_cast<$underlying$>(value));\n"
                "  return idx == -1 ? ::$proto_ns$::internal::GetEmptyString() :\n"
                "                     $classname$_strings[idx].get();\n"
                "}\n",
                CountUniqueValues(descriptor_));
        }

        if (HasDescriptorMethods(descriptor_->file(), options_)) {
            format(
                "bool $classtype$Trait::enum_parse(\n"
                "    ::SHARK_NAMESPACE_ID::ConstStringParam name, $classtype$* "
                "value) "
                "{\n"
                "  return ::$proto_ns$::internal::ParseNamedEnum<$classtype$>(\n"
                "    descriptor(), name, value);\n"
                "}\n");
        } else {
            format(
                "bool $classtype$Trait::enum_parse(\n"
                "    ::SHARK_NAMESPACE_ID::ConstStringParam name, $classtype$* "
                "value) "
                "{\n"
                "  int int_value;\n"
                "  bool success = ::$proto_ns$::internal::LookUpEnumValue(\n"
                "      $classname$_entries, $1$, name, &int_value);\n"
                "  if (success) {\n"
                "    *value = static_cast<$classtype$>(int_value);\n"
                "  }\n"
                "  return success;\n"
                "}\n",
                descriptor_->value_count());
        }
    }

    void EnumGenerator::generate_template_methods(int idx, io::Printer* printer) {
        Formatter format(printer, variables_);
        /*
        if (HasDescriptorMethods(descriptor_->file(), options_)) {

            format("const ::$proto_ns$::EnumDescriptor* EnumTraitImpl<$classtype$>::descriptor() {\n"
                "    ::$proto_ns$::internal::AssignDescriptors(&$desc_table$);\n"
                "    return $file_level_enum_descriptors$[$1$];\n"
                "}\n",
                idx);
        }

        format(
            "bool EnumTraitImpl<$classtype$>::is_valid(int64_t value) {\n"
            "  switch (value) {\n");

        // Multiple values may have the same number.  Make sure we only cover
        // each number once by first constructing a set containing all valid
        // numbers, then printing a case statement for each element.

        std::set<int> numbers;
        for (int j = 0; j < descriptor_->value_count(); j++) {
            const EnumValueDescriptor* value = descriptor_->value(j);
            numbers.insert(value->number());
        }

        for (std::set<int>::iterator iter = numbers.begin(); iter != numbers.end();
            ++iter) {
            format("    case $1$:\n", Int32ToString(*iter));
            }

        format(
            "      return true;\n"
            "    default:\n"
            "      return false;\n"
            "  }\n"
            "}\n"
            "\n");

        ////////////////////////////////////////////////
        ///
        if (!HasDescriptorMethods(descriptor_->file(), options_)) {
            // In lite mode (where descriptors are unavailable), we generate separate
            // tables for mapping between enum names and numbers. The _entries table
            // contains the bulk of the data and is sorted by name, while
            // _entries_by_number is sorted by number and just contains pointers into
            // _entries. The two tables allow mapping from name to number and number to
            // name, both in time logarithmic in the number of enum entries. This could
            // probably be made faster, but for now the tables are intended to be simple
            // and compact.
            //
            // Enums with allow_alias = true support multiple entries with the same
            // numerical value. In cases where there are multiple names for the same
            // number, we treat the first name appearing in the .shark file as the
            // canonical one.
            std::map<std::string, int> name_to_number;
            std::map<int, std::string> number_to_canonical_name;
            for (int i = 0; i < descriptor_->value_count(); i++) {
                const EnumValueDescriptor* value = descriptor_->value(i);
                name_to_number.emplace(value->name(), value->number());
                // The same number may appear with multiple names, so we use emplace() to
                // let the first name win.
                number_to_canonical_name.emplace(value->number(), value->name());
            }

            format(
                "static ::$proto_ns$::internal::ExplicitlyConstructed<std::string> "
                "$classname$_strings[$1$] = {};\n\n",
                CountUniqueValues(descriptor_));

            // We concatenate all the names for a given enum into one big string
            // literal. If instead we store an array of string literals, the linker
            // seems to put all enum strings for a given .shark file in the same
            // section, which hinders its ability to strip out unused strings.
            format("static const char $classname$_names[] =");
            for (const auto& p : name_to_number) {
                format("\n  \"$1$\"", p.first);
            }
            format(";\n\n");

            format(
                "static const ::$proto_ns$::internal::EnumEntry $classname$_entries[] "
                "= {\n");
            int i = 0;
            std::map<int, int> number_to_index;
            int data_index = 0;
            for (const auto& p : name_to_number) {
                format("  { {$classname$_names + $1$, $2$}, $3$ },\n", data_index,
                    p.first.size(), p.second);
                if (number_to_canonical_name[p.second] == p.first) {
                    number_to_index.emplace(p.second, i);
                }
                ++i;
                data_index += p.first.size();
            }

            format(
                "};\n"
                "\n"
                "static const int $classname$_entries_by_number[] = {\n");
            for (const auto& p : number_to_index) {
                format("  $1$, // $2$ -> $3$\n", p.second, p.first,
                    number_to_canonical_name[p.first]);
            }
            format(
                "};\n"
                "\n");
        }

        ////////////////////////////////////////////////
        ///
        if (HasDescriptorMethods(descriptor_->file(), options_)) {
            format("const std::string& EnumTraitImpl<$classtype$>::enum_name(\n"
                "   $classtype$ enum_t_value) {\n"
                "  return ::$proto_ns$::internal::NameOfEnum(\n"
                "    $classtype$Trait::descriptor(),\n"
                "    static_cast<$underlying$>(enum_t_value));\n");
            format("}\n");
        } else {
            format(
                "const std::string& EnumTraitImpl<$classtype$>::enum_name(\n"
                "    $classtype$ value) {\n"
                "  static const bool dummy =\n"
                "      ::$proto_ns$::internal::InitializeEnumStrings(\n"
                "          $classname$_entries,\n"
                "          $classname$_entries_by_number,\n"
                "          $1$, $classname$_strings);\n"
                "  (void) dummy;\n"
                "  int idx = ::$proto_ns$::internal::LookUpEnumName(\n"
                "      $classname$_entries,\n"
                "      $classname$_entries_by_number,\n"
                "      $1$, static_cast<$underlying$>(value));\n"
                "  return idx == -1 ? ::$proto_ns$::internal::GetEmptyString() :\n"
                "                     $classname$_strings[idx].get();\n"
                "}\n",
                CountUniqueValues(descriptor_));
        }

        if (HasDescriptorMethods(descriptor_->file(), options_)) {
            format(
                "bool EnumTraitImpl<$classtype$>::enum_parse(\n"
                "    ::SHARK_NAMESPACE_ID::ConstStringParam name, $classtype$* "
                "value) "
                "{\n"
                "  return ::$proto_ns$::internal::ParseNamedEnum<$classtype$>(\n"
                "    descriptor(), name, value);\n"
                "}\n");
        } else {
            format(
                "bool EnumTraitImpl<$classtype$>::enum_parse(\n"
                "    ::SHARK_NAMESPACE_ID::ConstStringParam name, $classtype$* "
                "value) "
                "{\n"
                "  int int_value;\n"
                "  bool success = ::$proto_ns$::internal::LookUpEnumValue(\n"
                "      $classname$_entries, $1$, name, &int_value);\n"
                "  if (success) {\n"
                "    *value = static_cast<$classtype$>(int_value);\n"
                "  }\n"
                "  return success;\n"
                "}\n",
                descriptor_->value_count());
        }
        */
    }

} // namespace shark::compiler::cpp
