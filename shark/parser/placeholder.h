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
#include <shark/parser/symbol.h>
#include <cstddef>

namespace shark::parser {

    class PlaceholderParser : public SymbolNode {
    public:
        enum class Constraint {
            KEY,
            ALL,
        };

        PlaceholderParser(const FileParser* f, RegistryPool* pool)
            : SymbolNode(SymbolType::PLACEHOLDER, f, pool) {}

        bool allows(const SymbolNode* symbol) const;

        Constraint constraint{Constraint::ALL};
        const PhysicalType* allowed{nullptr};
        std::size_t allowed_count{0};
    };

}  // namespace shark::parser
