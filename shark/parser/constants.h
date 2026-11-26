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

namespace shark::parser {

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

    // Field qualifier. The four values do not combine.
    // CONSTEXPR requires a ConstValueParser and is not serialized.
    // ATOMIC has the same presence as REQUIRED and is accessed atomically.
    enum class FieldLabel : uint8_t {
        REQUIRED,
        OPTIONAL,
        CONSTEXPR,
        ATOMIC,
    };

    enum class NodeType {
        NONE = 0,
        FILE,
        EDITION,
        COMMENT,
        PACKAGE,
        IMPORT,
        ANNOTATION,
        CONST,
        SYMBOL_MIN,
        ENUM_VALUE = SYMBOL_MIN,
        ENUM,
        STRUCT,
        FIELD,
        FUNCTION,
        SERVLET,
        ALIAS,
        PLACEHOLDER,
        SYMBOL_MAX = PLACEHOLDER
    };
}  // namespace shark::parser
