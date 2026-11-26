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

#include <shark/parser/builtin.h>
#include <shark/parser/pool.h>
#include <shark/parser/file.h>
#include <shark/common/clog.h>
#include <shark/common/filepath.h>
#include <turbo/log/logging.h>
#include <string>

namespace shark::parser {

    RegistryPool::RegistryPool() {
        load_builtin_types(*this);
    }

    void RegistryPool::add_file(FileParser *ptr) {
        std::string_view path = ptr->path;
        _files[path] = ptr;
    }

    FileParser* RegistryPool::build_file(std::string_view file) {
        std::string prefix;
        auto rel = shark::common::relative_path(file, includes(), &prefix);
        if (!rel.ok()) {
            shark::common::report_error("file `" + std::string(file) + "` was not found under include paths");
            return nullptr;
        }

        const std::string absolute = prefix + "/" + *rel;
        if (FileParser* existing = get_file(*rel)) {
            return existing;
        }

        auto* node = create_symbol<FileParser>(nullptr);
        if (!node->load(absolute, *rel)) {
            return nullptr;
        }
        add_file(node);
        if (!node->parse()) {
            return nullptr;
        }
        return node;
    }

    bool RegistryPool::link_imports(FileParser* node) {
        for (ImportParser* import_node : node->imports) {
            if (!import_node->classify.empty()) {
                continue;
            }

            std::string_view raw = import_node->path;
            if (raw.size() < 2) {
                shark::common::report_error("import path in `" + node->path + "` is empty");
                return false;
            }
            std::string_view path = raw.substr(1, raw.size() - 2);

            auto found = shark::common::absolute_path(path, includes());
            if (!found.ok()) {
                shark::common::report_error("import `" + std::string(path) + "` from `" + node->path + "` was not found under include paths");
                return false;
            }

            std::string prefix;
            auto rel = shark::common::relative_path(*found, includes(), &prefix);
            if (!rel.ok()) {
                shark::common::report_error("import `" + std::string(path) + "` from `" + node->path + "` was not found under include paths");
                return false;
            }

            if (FileParser* existing = get_file(*rel)) {
                if (!_import_graph.add_dependency(node, existing)) {
                    shark::common::report_error("import `" + std::string(path) + "` from `" + node->path + "` forms a cycle");
                    return false;
                }
                continue;
            }

            auto* dep = create_symbol<FileParser>(nullptr);
            if (!dep->load(*found, *rel)) {
                return false;
            }
            add_file(dep);
            if (!dep->parse()) {
                return false;
            }
            _import_graph.add_file(dep);
            if (!_import_graph.add_dependency(node, dep)) {
                return false;
            }
            if (!link_imports(dep)) {
                return false;
            }
        }

        return true;
    }

    FileParser* RegistryPool::build_file_recursive(std::string_view file) {
        _files_queue.clear();

        std::string prefix;
        auto rel = shark::common::relative_path(file, includes(), &prefix);
        if (!rel.ok()) {
            KLOG(INFO) << rel;
            return nullptr;
        }

        FileParser* node = get_file(*rel);
        if (node == nullptr) {
            const std::string absolute = prefix + "/" + *rel;
            node = create_symbol<FileParser>(nullptr);
            if (!node->load(absolute, *rel)) {
                return nullptr;
            }
            add_file(node);
            if (!node->parse()) {
                return nullptr;
            }
        }
        _import_graph.add_file(node);

        if (!link_imports(node)) {
            return nullptr;
        }
        if (!_import_graph.build_queue()) {
            shark::common::report_debug("import queue failed");
            _files_queue.clear();
            return nullptr;
        }
        _files_queue.assign(_import_graph.queue().begin(), _import_graph.queue().end());
        int index = 1;
        for (FileParser* queued : _files_queue) {
            shark::common::report_debug("bind " + std::to_string(index) + " `" + queued->path + "`");
            for (FileParser* dep : _import_graph.direct(queued)) {
                shark::common::report_debug("\t- `" + dep->path + "`");
            }
            ++index;
        }
        for (FileParser* queued : _files_queue) {
            if (!queued->bind_file()) {
                return nullptr;
            }
        }
        bool bound = true;
        for (FileParser* queued : _files_queue) {
            if (queued->bind) {
                continue;
            }
            bound = false;
            shark::common::report_debug("unbound file `" + queued->path + "`");
        }
        for (const auto& symbol : _all_symbols) {
            if (symbol->node_type == SymbolType::FILE || symbol->bind) {
                continue;
            }
            bound = false;
            std::string label = symbol->full_name.empty() ? std::string(symbol->name) : symbol->full_name;
            shark::common::report_debug("unbound symbol `" + label + "`");
        }
        if (bound) {
            shark::common::report_debug("bind success");
        }
        return node;
    }
}  // namespace shark::parser
