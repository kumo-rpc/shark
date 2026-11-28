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
#include <shark/compiler/cpp/struct.h>
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

}  // namespace shark::compiler::cpp
