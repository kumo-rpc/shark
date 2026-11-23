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

#include <shark/parser/symbol.h>
#include <shark/parser/file.h>
#include <shark/parser/symbol.h>
#include <shark/parser/pool.h>

namespace shark::parser {

    bool SymbolNode::emit_to_pool() {
        auto ptr= pool->get_symbol(full_name);
        if (ptr != nullptr) {
            return false;
        }
        auto r=  pool->add_symbol(this);
        if (r) {
            emit = true;
        }
        return r;
    }
}  // namespace shark::parser

