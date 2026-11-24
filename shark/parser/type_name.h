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
#include <variant>
#include <vector>

namespace shark::parser {

    class Node;
    class ExpressionLexer;
    template <typename Lex>
    class Lexer;

    /// A type written in source. Plain names stay in `string_view`.
    /// A generic use stores nested arguments in `TypeName`.
    struct TypeName {
        struct Arg;

        TypeName();
        ~TypeName();
        TypeName(const TypeName& other);
        TypeName& operator=(const TypeName& other);
        TypeName(TypeName&& other) noexcept;
        TypeName& operator=(TypeName&& other) noexcept;

        std::string_view name;
        std::vector<Arg> args;
    };

    struct TypeName::Arg {
        std::variant<std::string_view, TypeName> value;
    };

    using TypeRef = std::variant<std::string_view, TypeName>;

    inline std::string_view type_ref_name(const TypeRef& ref) {
        if (const auto* plain = std::get_if<std::string_view>(&ref)) {
            return *plain;
        }
        return std::get<TypeName>(ref).name;
    }

    std::string format_type_ref(const TypeRef& ref);

    // Reads a dotted name and optional `<...>` arguments. The caller owns errors.
    bool parse_type_ref(Node& node, Lexer<ExpressionLexer>& lexer, TypeRef& out);

}  // namespace shark::parser
