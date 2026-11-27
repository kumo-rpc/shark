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

#include <shark/common/filepath.h>
#include <turbo/log/logging.h>
#include <deque>
#include <fermat/maps/btree_map.h>
#include <fermat/maps/btree_set.h>
#include <shark/parser/import_graph.h>
#include <shark/parser/symbol.h>
#include <shark/schema/schema_pool.h>

namespace shark::parser {

    class RegistryPool {
    public:
        static RegistryPool& instance() {
            static RegistryPool ins;
            return ins;
        }

        /// no lock, parser always single thread
        [[nodiscard]] bool has_symbol(std::string_view url) const {
            auto it = _symbols.find(url);
            if (it == _symbols.end()) {
                return false;
            }
            return true;
        }
        [[nodiscard]] bool add_symbol(SymbolNode* ptr) {
            std::string_view uri = ptr->full_name;
            auto it = _symbols.find(uri);
            if (it != _symbols.end()) {
                return false;
            }
            _symbols[uri] = ptr;
            return true;
        }

        [[nodiscard]] const SymbolNode* get_symbol(std::string_view url) const {
            auto it = _symbols.find(url);
            if (it == _symbols.end()) {
                return nullptr;
            }
            return it->second;
        }

        [[nodiscard]] SymbolNode* get_symbol(std::string_view url) {
            auto it = _symbols.find(url);
            if (it == _symbols.end()) {
                return nullptr;
            }
            return it->second;
        }

        void add_file(FileParser *ptr);

        [[nodiscard]] bool has_file(std::string_view path) {
            auto it = _files.find(path);
            if (it == _files.end()) {
                return false;
            }
            return true;
        }

        [[nodiscard]] const FileParser* get_file(std::string_view path) const {
            auto it = _files.find(path);
            if (it == _files.end()) {
                return nullptr;
            }
            return it->second;
        }

        [[nodiscard]] FileParser* get_file(std::string_view path) {
            auto it = _files.find(path);
            if (it == _files.end()) {
                return nullptr;
            }
            return it->second;
        }

        void add_include(std::string_view path) {
            if (_includes_set.find(path) != _includes_set.end()) {
                return;
            }
            _includes_set.insert(std::string(path));
            _includes.push_back(std::string(path));
        }

        [[nodiscard]] const std::vector<std::string>& includes() const {
            return _includes;
        }

       [[nodiscard]] std::vector<SymbolNode*> not_emit_symbol() const {
            std::vector<SymbolNode*> ret;
            for (auto &it : _all_symbols) {
                if (!it->emit) {
                    ret.push_back(it.get());
                }
            }
            return ret;
        }

        template <typename T>
        T* create_symbol(const FileParser* f) {
            static_assert(std::is_base_of_v<SymbolNode, T>, "must be");
            auto npr = new T(f, this);
            std::unique_ptr<T> ptr(npr);
            auto ret = ptr.get();
            _all_symbols.push_back(std::move(ptr));
            return ret;
        }

        template <typename T>
        T* create_node(const FileParser* f) {
            static_assert(std::is_base_of_v<Node, T>, "must be");
            static_assert(!std::is_base_of_v<SymbolNode, T>, "must be");
            auto npr = new T(f, this);
            std::unique_ptr<T> ptr(npr);
            auto ret = ptr.get();
            _all_node.push_back(std::move(ptr));
            return ret;
        }


        SchemaPoolBase& schema_pool() {
            return schema_pool_;
        }

        FileParser* build_file(std::string_view file);

        FileParser* build_file(std::string_view file, const std::vector<std::string> &includes) {
            for (const auto &it : includes) {
                auto r = shark::common::absolute_path(it);
                if (!r.ok()) {
                    KLOG(ERROR)<<r;
                }
                add_include(r.value());
            }
            return build_file(file);
        }

        FileParser* build_file_recursive(std::string_view file);

        FileParser* build_file_recursive(std::string_view file, const std::vector<std::string> &includes) {
            for (const auto &it : includes) {
                auto r = shark::common::absolute_path(it);
                if (!r.ok()) {
                    KLOG(ERROR)<<r;
                }
                add_include(r.value());
            }
            return build_file_recursive(file);
        }

    private:
        RegistryPool();
        bool link_imports(FileParser* node);
        std::vector<std::unique_ptr<SymbolNode>>           _all_symbols;
        std::vector<std::unique_ptr<Node>>                  _all_node;
        fermat::btree_map<std::string_view, SymbolNode*>    _symbols;
        fermat::btree_map<std::string_view, FileParser*>      _files;
        fermat::btree_set<std::string>                      _includes_set;
        std::vector<std::string>                            _includes;
        SchemaPoolBase                                      schema_pool_;
        std::deque<FileParser*>                               _files_queue;
        ImportGraph                                         _import_graph;
    };

}  // namespace shark::parser
