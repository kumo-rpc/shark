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

#include <shark/parser/fwd.h>
#include <shark/parser/lexer.h>
#include <shark/parser/node.h>

#include <cstdint>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace shark::parser {

    // A parsed constant node. Not a symbol.
    // Strings and identifiers view the source buffer.
    // List and map elements are pool-allocated ConstValueParser nodes.
    class ConstValueParser : public Node {
    public:
        struct List {
            std::vector<ConstValueParser*> elements;
        };

        struct Map {
            std::vector<std::pair<ConstValueParser*, ConstValueParser*>> entries;
        };

        struct Identifier {
            std::string_view name;
            const EnumParser* enum_type{nullptr};
        };

        using Value = std::variant<
            std::monostate,
            bool,
            int64_t,
            double,
            std::string_view,
            Identifier,
            List,
            Map>;

        ConstValueParser(const FileParser* f, RegistryPool* p)
            : Node(SymbolType::CONST, f, p) {}

        // Parses one value. Skips whitespace and comments.
        // Does not consume a trailing comma or semicolon.
        bool parse(Lexer<ExpressionLexer>& lexer);

        Value value;

    private:
        bool skip(Lexer<ExpressionLexer>& lexer);
        bool parse_number(Lexer<ExpressionLexer>& lexer, bool negative);
        bool parse_identifier(Lexer<ExpressionLexer>& lexer);
        bool parse_list(Lexer<ExpressionLexer>& lexer);
        bool parse_map(Lexer<ExpressionLexer>& lexer);
    };

} // namespace shark::parser
