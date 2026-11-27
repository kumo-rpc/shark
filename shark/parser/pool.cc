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
#include <shark/parser/import.h>
#include <shark/parser/pool.h>
#include <shark/parser/file.h>
#include <shark/common/clog.h>
#include <shark/common/filepath.h>
#include <turbo/log/logging.h>
#include <filesystem>
#include <string>

namespace shark::parser {

    namespace {

        constexpr std::string_view kStdRel = "shark/schema/std.shark";
        constexpr std::string_view kStdImport = "\"shark/schema/std.shark\"";

        bool is_std_import(std::string_view raw) {
            if (raw.size() < 2) {
                return false;
            }
            return raw.substr(1, raw.size() - 2) == kStdRel;
        }

    }  // namespace

    RegistryPool::RegistryPool() = default;

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

    FileParser* RegistryPool::load_rel(std::string_view rel) {
        if (FileParser* existing = get_file(rel)) {
            return existing;
        }
        std::string prefix;
        auto found = shark::common::absolute_path(rel, includes());
        if (!found.ok()) {
            shark::common::report_error("file `" + std::string(rel) + "` was not found under include paths");
            return nullptr;
        }
        auto resolved = shark::common::relative_path(*found, includes(), &prefix);
        if (!resolved.ok()) {
            shark::common::report_error("file `" + std::string(rel) + "` was not found under include paths");
            return nullptr;
        }
        const std::string absolute = prefix + "/" + *resolved;
        auto* node = create_symbol<FileParser>(nullptr);
        if (!node->load(absolute, *resolved)) {
            return nullptr;
        }
        add_file(node);
        if (!node->parse()) {
            return nullptr;
        }
        return node;
    }

    bool RegistryPool::load_std() {
        std::filesystem::path source = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
        add_include(source.string());
        FileParser* std_file = load_rel(kStdRel);
        if (std_file == nullptr) {
            return false;
        }
        _builtin_files.insert(std_file);
        _import_graph.add_file(std_file);
        return link_imports(std_file);
    }

    bool RegistryPool::link_imports(FileParser* node) {
        if (_builtin_files.find(node) == _builtin_files.end()) {
            bool has_std = false;
            for (ImportParser* import_node : node->imports) {
                if (is_std_import(import_node->path)) {
                    has_std = true;
                    break;
                }
            }
            if (!has_std) {
                auto* import_node = create_node<ImportParser>(node);
                import_node->path = kStdImport;
                node->imports.push_back(import_node);
            }
        }
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
            if (_builtin_files.find(node) != _builtin_files.end()) {
                _builtin_files.insert(dep);
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

        if (!load_std()) {
            return nullptr;
        }
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
