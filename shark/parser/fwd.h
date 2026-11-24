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


#include <shark/parser/constants.h>

namespace shark::parser {

    class EnumValueNode;
    class EnumNode;
    class FileNode;
    class StructNode;
    class RegistryPool;
    class FieldNode;
    class AliasNode;
    class ServiceNode;
    class ServletNode;
    class FunctionNode;
    class ConstValue;

    enum class PhysicalType {
        VOID    = 0,
        BOOL    = 1,
        INT8    = 2,
        UINT8   = 3,
        INT16   = 4,
        UINT16  = 5,
        INT32   = 6,
        UINT32  = 7,
        INT64   = 8,
        UINT64  = 9,
        INT128  = 10,
        UINT128 = 11,
        FLOAT32 = 12,
        FLOAT64 = 13,
        STRING  = 14,
        STRUCT  = 15,
        CONTAINER_MIN = 16,
        LIST    = CONTAINER_MIN,
        SET     = 17,
        MAP     = 18,
        OPTIONAL = 19,
        CONTAINER_MAX = MAP,
    };


}  // namespace shark::parser
