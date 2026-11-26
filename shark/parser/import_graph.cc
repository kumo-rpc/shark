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

#include <shark/parser/import_graph.h>
#include <algorithm>
#include <deque>

namespace shark::parser {

    namespace {

        const std::vector<FileParser*>& empty_files() {
            static const std::vector<FileParser*> kEmpty;
            return kEmpty;
        }

        bool contains_file(const std::vector<FileParser*>& files, FileParser* file) {
            return std::find(files.begin(), files.end(), file) != files.end();
        }

    }  // namespace

    void ImportGraph::add_file(FileParser* file) {
        if (file == nullptr) {
            return;
        }
        auto& entry = _nodes[file];
        entry.file = file;
    }

    bool ImportGraph::add_dependency(FileParser* from, FileParser* to) {
        if (from == nullptr || to == nullptr || from == to) {
            return false;
        }
        add_file(from);
        add_file(to);
        auto& from_entry = _nodes[from];
        if (contains_file(from_entry.direct, to)) {
            return true;
        }
        from_entry.direct.push_back(to);
        _nodes[to].dependents.push_back(from);
        return true;
    }

    const ImportGraph::Entry* ImportGraph::find(FileParser* file) const {
        auto it = _nodes.find(file);
        if (it == _nodes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    const std::vector<FileParser*>& ImportGraph::direct(FileParser* file) const {
        const Entry* entry = find(file);
        if (entry == nullptr) {
            return empty_files();
        }
        return entry->direct;
    }

    const std::vector<FileParser*>& ImportGraph::dependents(FileParser* file) const {
        const Entry* entry = find(file);
        if (entry == nullptr) {
            return empty_files();
        }
        return entry->dependents;
    }

    bool ImportGraph::build_queue() {
        _queue.clear();

        fermat::btree_map<FileParser*, int> pending;
        std::deque<FileParser*> ready;
        for (auto& it : _nodes) {
            int count = static_cast<int>(it.second.direct.size());
            pending[it.first] = count;
            if (count == 0) {
                ready.push_back(it.first);
            }
        }

        while (!ready.empty()) {
            FileParser* file = ready.front();
            ready.pop_front();
            _queue.push_back(file);
            for (FileParser* user : _nodes[file].dependents) {
                int& left = pending[user];
                --left;
                if (left == 0) {
                    ready.push_back(user);
                }
            }
        }

        if (_queue.size() != _nodes.size()) {
            _queue.clear();
            return false;
        }
        return true;
    }

}  // namespace shark::parser
