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
#include <deque>
#include <fermat/maps/btree_map.h>
#include <vector>

namespace shark::parser {

    /// Direct import edges between files. FileParser lifetime stays in RegistryPool.
    class ImportGraph {
    public:
        void add_file(FileParser* file);

        /// One direct edge. The same pair is stored once. A self-edge fails.
        bool add_dependency(FileParser* from, FileParser* to);

        [[nodiscard]] const std::vector<FileParser*>& direct(FileParser* file) const;
        [[nodiscard]] const std::vector<FileParser*>& dependents(FileParser* file) const;

        /// Files with no direct imports come first. Fails when the graph has a cycle.
        bool build_queue();

        [[nodiscard]] const std::deque<FileParser*>& queue() const {
            return _queue;
        }

    private:
        struct Entry {
            FileParser* file{nullptr};
            std::vector<FileParser*> direct;
            std::vector<FileParser*> dependents;
        };

        [[nodiscard]] const Entry* find(FileParser* file) const;

        fermat::btree_map<FileParser*, Entry> _nodes;
        std::deque<FileParser*> _queue;
    };

}  // namespace shark::parser
