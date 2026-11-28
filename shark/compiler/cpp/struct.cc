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
#include <shark/compiler/cpp/constexpr.h>
#include <shark/compiler/cpp/enum.h>
#include <shark/compiler/cpp/struct.h>
#include <shark/schema/constrain.h>
#include <shark/schema/schema_defines.h>

#include <map>
#include <string>

#include <turbo/strings/str_replace.h>

namespace shark::compiler::cpp {
namespace {

std::string alias_type_name(const SchemaType* type) {
    if (type == nullptr) {
        return {};
    }
    for (uint32_t i = 0; i < type->annotation_size(); ++i) {
        const SchemaAnnotation* annotation = type->annotation(i);
        if (annotation == nullptr || annotation->classify() != kCppClassify || annotation->key() != kTypeKey) {
            continue;
        }
        const SchemaConstValue* value = annotation->value();
        if (value == nullptr || value->kind() != SchemaConstValue::Kind::STRING) {
            continue;
        }
        return value->string_value();
    }
    return turbo::StrReplaceAll(type->full_name_without_package(), {{".", "::"}});
}

}  // namespace

    void StructGenerator::generate_fwd_declare(io::Printer& printer) {
        if (_struct->container() != nullptr || _struct->generic()) {
            return;
        }
        const SchemaTypeName* type_name = _struct->type_name();
        if (type_name == nullptr || type_name->name().empty()) {
            return;
        }
        std::map<std::string, std::string> vars;
        vars["name"] = type_name->name();
        printer.print(vars, _struct->is_union() ? "union $name$;\n" : "class $name$;\n");
    }

    void StructGenerator::generate_contained_fwd(io::Printer& printer) {
        const SchemaTypeName* type_name = _struct->type_name();
        if (type_name == nullptr || type_name->name().empty()) {
            return;
        }
        std::map<std::string, std::string> vars;
        vars["name"] = type_name->name();
        if (_struct->generic()) {
            if (type_name->argument_size() == 0) {
                return;
            }
            std::string params;
            for (uint32_t i = 0; i < type_name->argument_size(); ++i) {
                const SchemaTypeName* argument = type_name->argument(i);
                if (argument == nullptr || argument->name().empty()) {
                    return;
                }
                if (!params.empty()) {
                    params.append(", ");
                }
                params.append("typename ");
                params.append(argument->name());
            }
            vars["params"] = params;
            printer.print(vars, "template <$params$>\n");
        }
        printer.print(vars, _struct->is_union() ? "union $name$;\n" : "class $name$;\n");
    }

    void StructGenerator::generate_generic_fwd_declare(io::Printer& printer) {
        if (_struct->container() != nullptr || !_struct->generic()) {
            return;
        }
        const SchemaTypeName* type_name = _struct->type_name();
        if (type_name == nullptr || type_name->name().empty() || type_name->argument_size() == 0) {
            return;
        }
        std::string params;
        for (uint32_t i = 0; i < type_name->argument_size(); ++i) {
            const SchemaTypeName* argument = type_name->argument(i);
            if (argument == nullptr || argument->name().empty()) {
                return;
            }
            if (!params.empty()) {
                params.append(", ");
            }
            params.append("typename ");
            params.append(argument->name());
        }
        std::map<std::string, std::string> vars;
        vars["params"] = params;
        vars["name"] = type_name->name();
        printer.print(vars, "template <$params$>\n");
        printer.print(vars, _struct->is_union() ? "union $name$;\n" : "class $name$;\n");
    }

    void StructGenerator::generate_alias_declare(io::Printer& printer) {
        if (_struct->container() != nullptr) {
            return;
        }
        for (uint32_t i = 0; i < _struct->alias_size(); ++i) {
            const SchemaTypeAlias* alias = _struct->alias(i);
            if (alias == nullptr || alias->name().empty()) {
                continue;
            }
            const std::string type = alias_type_name(alias->target());
            if (type.empty()) {
                shark::common::report_error(alias->full_name() + " alias target is missing");
                continue;
            }
            std::map<std::string, std::string> vars;
            vars["name"] = alias->name();
            vars["type"] = type;
            printer.print(vars, alias->is_using() ? "using $name$ = $type$;\n" : "typedef $type$ $name$;\n");
        }
    }

    void StructGenerator::generate_define(io::Printer& printer) {
        if (_struct->container() != nullptr) {
            return;
        }
        const SchemaTypeName* type_name = _struct->type_name();
        if (type_name == nullptr || type_name->name().empty()) {
            return;
        }
        std::map<std::string, std::string> vars;
        vars["name"] = type_name->name();
        if (_struct->generic()) {
            if (type_name->argument_size() == 0) {
                return;
            }
            std::string params;
            for (uint32_t i = 0; i < type_name->argument_size(); ++i) {
                const SchemaTypeName* argument = type_name->argument(i);
                if (argument == nullptr || argument->name().empty()) {
                    return;
                }
                if (!params.empty()) {
                    params.append(", ");
                }
                params.append("typename ");
                params.append(argument->name());
            }
            vars["params"] = params;
            printer.print(vars, "template <$params$>\n");
        }
        printer.print(vars, _struct->is_union() ? "union $name$ {\n" : "class $name$ {\n");
        printer.indent();
        for (uint32_t i = 0; i < _struct->field_size(); ++i) {
            const SchemaField* field = _struct->field(i);
            if (field == nullptr || field->label() != FieldLabel::CONSTEXPR || field->type() == nullptr) {
                continue;
            }
            if (!type_is_primitive(field->type()) || !type_is_constexpr(field->type())) {
                continue;
            }
            ConstexprGenerator generator(field);
            generator.generate_primitive_constexpr_declare(printer);
        }
        for (uint32_t i = 0; i < _struct->nested_enum_size(); ++i) {
            const SchemaEnum* item = _struct->nested_enum(i);
            if (item == nullptr) {
                continue;
            }
            EnumGenerator generator(item);
            generator.generate_nested_declare(printer);
        }
        for (uint32_t i = 0; i < _struct->field_size(); ++i) {
            const SchemaField* field = _struct->field(i);
            if (field == nullptr || field->label() != FieldLabel::CONSTEXPR) {
                continue;
            }
            ConstexprGenerator generator(field);
            generator.generate_enum_constexpr_declare(printer);
        }
        for (uint32_t i = 0; i < _struct->nested_type_size(); ++i) {
            const SchemaStruct* nested = _struct->nested_type(i);
            if (nested == nullptr || nested->generic()) {
                continue;
            }
            StructGenerator generator(nested);
            generator.generate_contained_fwd(printer);
        }
        for (uint32_t i = 0; i < _struct->nested_type_size(); ++i) {
            const SchemaStruct* nested = _struct->nested_type(i);
            if (nested == nullptr || !nested->generic()) {
                continue;
            }
            StructGenerator generator(nested);
            generator.generate_contained_fwd(printer);
        }
        generate_alias_declare(printer);
        printer.outdent();
        printer.print("};\n");
    }

}  // namespace shark::compiler::cpp
