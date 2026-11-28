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

#include <shark/schema/pass_check.h>
#include <shark/common/clog.h>
#include <shark/schema/annotation.h>
#include <shark/schema/constrain.h>
#include <shark/schema/schema_pool_api.h>

#include <set>
#include <string>

namespace shark {
namespace {

bool check_enum_values(const SchemaFile* file, const SchemaEnum* item) {
    if (item == nullptr) {
        return true;
    }
    std::set<int64_t> seen;
    for (uint32_t i = 0; i < item->value_size(); ++i) {
        const SchemaEnumValue* value = item->value(i);
        if (value == nullptr) {
            continue;
        }
        if (!seen.insert(value->value()).second && !Annotations::allow_enum_alias(value)) {
            shark::common::report_error(file->path() + ":" + std::to_string(value->line()) + ": enum value `" + value->name() + "` = " + std::to_string(value->value()) + " duplicates an enum value");
            return false;
        }
    }
    return true;
}

bool check_struct_enums(const SchemaFile* file, const SchemaStruct* item) {
    if (item == nullptr) {
        return true;
    }
    for (uint32_t i = 0; i < item->nested_type_size(); ++i) {
        if (!check_struct_enums(file, item->nested_type(i))) {
            return false;
        }
    }
    for (uint32_t i = 0; i < item->nested_enum_size(); ++i) {
        if (!check_enum_values(file, item->nested_enum(i))) {
            return false;
        }
    }
    return true;
}

bool check_enum_alias(const SchemaFile* file) {
    if (file == nullptr) {
        return true;
    }
    for (uint32_t i = 0; i < file->enum_size(); ++i) {
        if (!check_enum_values(file, file->enum_at(i))) {
            return false;
        }
    }
    for (uint32_t i = 0; i < file->struct_size(); ++i) {
        if (!check_struct_enums(file, file->struct_at(i))) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool run_pass_check(const FileBase* file) {
    if (!run_constrain_check(file)) {
        return false;
    }
    return check_enum_alias(SchemaFile::from(file));
}

}  // namespace shark
