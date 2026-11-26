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

#include <string_view>
#include <string>

namespace shark {

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

    /// Field qualifier. The four values do not combine.
    /// CONSTEXPR requires a ConstValueParser and is not serialized.
    /// ATOMIC has the same presence as REQUIRED and is accessed atomically.
    enum class FieldLabel : uint8_t {
        REQUIRED,
        OPTIONAL,
        CONSTEXPR,
        ATOMIC,
    };



    static constexpr std::string_view kEditionKey = "edition";
    static constexpr std::string_view kImportKey = "import";
    static constexpr std::string_view kPackageKey = "package";
    static constexpr std::string_view kAnnotationKey = "annotation";
    static constexpr std::string_view kEnumKey = "enum";
    static constexpr std::string_view kStructKey = "struct";
    static constexpr std::string_view kUnionKey = "union";
    static constexpr std::string_view kUsingKey = "using";
    static constexpr std::string_view kTypeDefKey = "typedef";
    static constexpr std::string_view kFunctionKey = "function";
    static constexpr std::string_view kServletKey = "servlet";

    static constexpr std::string_view kRequiredKey    = "required";
    static constexpr std::string_view kOptionalKey    = "optional";
    static constexpr std::string_view kConstExprKey   = "constexpr";
    static constexpr std::string_view kAtomicKey      = "atomic";


    enum class SymbolType {
        NONE = 0,
        FILE,
        EDITION,
        COMMENT,
        PACKAGE,
        IMPORT,
        ANNOTATION,
        SYMBOL_MIN,
        ENUM_VALUE = SYMBOL_MIN,
        CONST,
        ENUM,
        STRUCT,
        FIELD,
        FUNCTION,
        SERVLET,
        ALIAS,
        PLACEHOLDER,
        SYMBOL_MAX = PLACEHOLDER
    };

    /// 1-based line and column of the declaration in the source file.
    /// Same meaning as shark::parser::SourceLocation.
    struct SourceLocation {
        int32_t line{0};
        int32_t column{0};
    };


}  // namespace shark
