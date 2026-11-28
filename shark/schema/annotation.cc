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

#include <shark/schema/annotation.h>
#include <shark/schema/schema_defines.h>

#include <set>

namespace shark {

std::string Annotations::cpp_include(const SchemaType* type) {
    if (type == nullptr) {
        return {};
    }
    for (uint32_t i = 0; i < type->annotation_size(); ++i) {
        const SchemaAnnotation* annotation = type->annotation(i);
        if (annotation == nullptr || annotation->classify() != kCppClassify || annotation->key() != kIncludeKey) {
            continue;
        }
        const SchemaConstValue* value = annotation->value();
        if (value == nullptr || value->kind() != SchemaConstValue::Kind::STRING) {
            continue;
        }
        return Annotations::normalize_include(value->string_value());
    }
    return {};
}

template <typename Node>
bool allow_alias_flag(const Node* node) {
    if (node == nullptr) {
        return false;
    }
    for (uint32_t i = 0; i < node->annotation_size(); ++i) {
        const SchemaAnnotation* annotation = node->annotation(i);
        if (annotation == nullptr || !annotation->classify().empty() || annotation->key() != kAllowEnumAliasKey) {
            continue;
        }
        const SchemaConstValue* value = annotation->value();
        if (value != nullptr && value->kind() == SchemaConstValue::Kind::BOOL && value->bool_value()) {
            return true;
        }
    }
    return false;
}

bool Annotations::allow_enum_alias(const SchemaEnum* item) {
    return allow_alias_flag(item);
}

bool Annotations::allow_enum_alias(const SchemaEnumValue* value) {
    if (allow_alias_flag(value)) {
        return true;
    }
    return value == nullptr ? false : allow_enum_alias(value->parent_enum());
}

std::string Annotations::normalize_include(std::string_view text) {
    while (text.size() >= 2) {
        const char open = text.front();
        const char close = text.back();
        const bool brackets = open == '<' && close == '>';
        const bool quotes = (open == '"' && close == '"') || (open == '\'' && close == '\'');
        if (!brackets && !quotes) {
            break;
        }
        text.remove_prefix(1);
        text.remove_suffix(1);
    }
    return std::string(text);
}

std::vector<std::string> Annotations::cpp_includes(const SchemaFile* file) {
    std::vector<std::string> headers;
    if (file == nullptr) {
        return headers;
    }
    std::set<std::string> seen;
    for (uint32_t i = 0; i < file->constexpr_size(); ++i) {
        const SchemaField* field = file->constexpr_at(i);
        if (field == nullptr) {
            continue;
        }
        const std::string header = cpp_include(field->type());
        if (header.empty() || !seen.insert(header).second) {
            continue;
        }
        headers.push_back(header);
    }
    return headers;
}

}  // namespace shark
