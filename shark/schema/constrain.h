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

#pragma once

#include <shark/schema/schema_pool.h>

namespace shark {

    class SchemaType;

    void register_constrain(SchemaPoolBase *pool);

    bool check_type_name(SchemaPoolBase* pool, const FileBase* file, const SourceLocation& location, const TypeName& type);

    bool bind_schema_types(FileBase* file);

    bool run_constrain_check(const FileBase *file);

    //////////////////////////////////////////////////////////////
    /// traits operators
    bool is_integer(const TypeInfo *type);
    bool is_void(const TypeInfo *type);
    bool is_primitive(const TypeInfo *type);
    bool is_pod(const TypeInfo *type);
    bool is_signed(const TypeInfo *type);
    bool is_numeric(const TypeInfo *type);
    bool is_container(const TypeInfo *type);
    bool is_string(const TypeInfo *type);
    bool is_constexpr(const TypeInfo *type);
    bool is_list(const TypeInfo *type);
    bool is_map(const TypeInfo *type);
    bool is_set(const TypeInfo *type);

    bool type_is_integer(const SchemaType *type);
    bool type_is_void(const SchemaType *type);
    bool type_is_primitive(const SchemaType *type);
    bool type_is_pod(const SchemaType *type);
    bool type_is_signed(const SchemaType *type);
    bool type_is_numeric(const SchemaType *type);
    bool type_is_container(const SchemaType *type);
    bool type_is_string(const SchemaType *type);
    bool type_is_constexpr(const SchemaType *type);
    bool type_is_list(const SchemaType *type);
    bool type_is_map(const SchemaType *type);
    bool type_is_set(const SchemaType *type);
}  // namespace shark
