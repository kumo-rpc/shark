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

    class SchemaStruct;
    class SchemaEnum;

    void register_constrain(SchemaPoolBase *pool);

    bool check_type_name(SchemaPoolBase* pool, const FileBase* file, const SourceLocation& location, const TypeName& type);

    bool bind_schema_types(FileBase* file);

    bool run_constrain_check(const FileBase *file);

    //////////////////////////////////////////////////////////////
    /// traits operators
    bool is_integer(const StructBase *type);
    bool is_void(const StructBase *type);
    bool is_primitive(const StructBase *type);
    bool is_pod(const StructBase *type);
    bool is_signed(const StructBase *type);
    bool is_numeric(const StructBase *type);
    bool is_container(const StructBase *type);
    bool is_string(const StructBase *type);
    bool is_constexpr(const StructBase *type);
    bool is_list(const StructBase *type);
    bool is_map(const StructBase *type);
    bool is_set(const StructBase *type);

    bool type_is_integer(const SchemaStruct *type);
    bool type_is_void(const SchemaStruct *type);
    bool type_is_primitive(const SchemaStruct *type);
    bool type_is_pod(const SchemaStruct *type);
    bool type_is_signed(const SchemaStruct *type);
    bool type_is_numeric(const SchemaStruct *type);
    bool type_is_container(const SchemaStruct *type);
    bool type_is_string(const SchemaStruct *type);
    bool type_is_constexpr(const SchemaStruct *type);
    bool type_is_list(const SchemaStruct *type);
    bool type_is_map(const SchemaStruct *type);
    bool type_is_set(const SchemaStruct *type);
    bool type_is_constexpr(const SchemaEnum *type);
}  // namespace shark
