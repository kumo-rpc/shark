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
#include <shark/compiler/cpp/function.h>
#include <shark/schema/schema_defines.h>

#include <map>
#include <string>

#include <turbo/strings/str_replace.h>

namespace shark::compiler::cpp {
namespace {

bool file_function(const SchemaFunction* item) {
    return item != nullptr && item->container() == nullptr && item->servlet() == nullptr && !item->name().empty();
}

std::string cpp_type_name(const SchemaType* type) {
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
    return {};
}

std::string type_spelling(const SchemaTypeName* written, const SchemaType* resolved) {
    const std::string annotated = cpp_type_name(resolved);
    if (!annotated.empty()) {
        return annotated;
    }
    if (written != nullptr && written->is_parameter()) {
        return written->name();
    }
    if (resolved != nullptr) {
        return turbo::StrReplaceAll(resolved->full_name_without_package(), {{".", "::"}});
    }
    return {};
}

bool build_signature(const SchemaFunction* item, std::map<std::string, std::string>& vars) {
    const std::string result = type_spelling(item->return_type(), item->ret_type());
    if (result.empty()) {
        shark::common::report_error(item->full_name() + " return type is missing");
        return false;
    }
    std::string args;
    for (uint32_t i = 0; i < item->param_size(); ++i) {
        const SchemaField* param = item->param(i);
        if (param == nullptr || param->name().empty()) {
            shark::common::report_error(item->full_name() + " parameter is missing");
            return false;
        }
        const std::string type = type_spelling(param->type_name(), param->type());
        if (type.empty()) {
            shark::common::report_error(item->full_name() + " parameter type is missing");
            return false;
        }
        if (!args.empty()) {
            args.append(", ");
        }
        args.append(type);
        args.push_back(' ');
        args.append(param->name());
    }
    vars["type"] = result;
    vars["name"] = item->name();
    vars["args"] = args;
    return true;
}

}  // namespace

void FunctionGenerator::generate_declare(io::Printer& printer) {
    if (!file_function(_function) || _function->parameter_size() != 0) {
        return;
    }
    std::map<std::string, std::string> vars;
    if (!build_signature(_function, vars)) {
        return;
    }
    printer.print(vars, "$type$ $name$($args$);\n");
}

void FunctionGenerator::generate_generic_declare(io::Printer& printer) {
    if (!file_function(_function) || _function->parameter_size() == 0) {
        return;
    }
    std::string params;
    for (uint32_t i = 0; i < _function->parameter_size(); ++i) {
        const SchemaTypeName* parameter = _function->parameter(i);
        if (parameter == nullptr || parameter->name().empty()) {
            shark::common::report_error(_function->full_name() + " type parameter is missing");
            return;
        }
        if (!params.empty()) {
            params.append(", ");
        }
        params.append("typename ");
        params.append(parameter->name());
    }
    std::map<std::string, std::string> vars;
    if (!build_signature(_function, vars)) {
        return;
    }
    vars["params"] = params;
    printer.print(vars, "template <$params$>\n$type$ $name$($args$);\n");
}

}  // namespace shark::compiler::cpp
