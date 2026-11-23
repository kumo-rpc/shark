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

#include <shark/parser/node.h>
#include <shark/parser/constants.h>
#include <shark/parser/fwd.h>
#include <string>

namespace shark::parser {

    class SymbolNode : public Node {
    public:

        bool emit_to_pool();

        mutable std::string      full_name;
        mutable std::string      full_name_without_package;
        bool                     emit{false};
        std::string_view         name;
        bool                     bind{false};

    protected:
        SymbolNode(NodeType type, const FileNode *f, RegistryPool *pool): Node(type, f, pool) {}
    };

}  // namespace shark::parser
