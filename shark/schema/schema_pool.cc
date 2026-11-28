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

#include <shark/schema/schema_pool.h>
#include <shark/schema/constrain.h>

#include <string>
#include <string_view>
#include <utility>

namespace shark {

    namespace {

        std::string_view registered_name(SymbolBase* symbol) {
            switch (symbol->symbol_type_) {
                case SymbolType::FILE:
                    return static_cast<FileBase*>(symbol)->full_name_;
                case SymbolType::ENUM_VALUE:
                    return static_cast<EnumValueBase*>(symbol)->full_name_;
                case SymbolType::ENUM:
                    return static_cast<EnumBase*>(symbol)->full_name_;
                case SymbolType::STRUCT:
                    return static_cast<StructBase*>(symbol)->full_name_;
                case SymbolType::FIELD:
                    return static_cast<FieldBase*>(symbol)->full_name_;
                case SymbolType::FUNCTION:
                    return static_cast<FunctionBase*>(symbol)->full_name_;
                case SymbolType::SERVLET:
                    return static_cast<ServletBase*>(symbol)->full_name_;
                case SymbolType::ALIAS:
                    return static_cast<AliasBase*>(symbol)->full_name_;
                default:
                    return {};
            }
        }

    }  // namespace

    SchemaPoolBase::SchemaPoolBase() {
        register_constrain(this);
    }

    bool SchemaPoolBase::add_symbol(SymbolBase* symbol) {
        if (symbol == nullptr) {
            return false;
        }
        std::string_view name = registered_name(symbol);
        if (name.empty()) {
            return false;
        }
        auto inserted = symbols_.insert(std::make_pair(std::string(name), symbol));
        return inserted.second;
    }

    bool SchemaPoolBase::add_symbol_name(std::string_view name, SymbolBase* symbol) {
        if (symbol == nullptr || name.empty()) {
            return false;
        }
        auto inserted = symbols_.insert(std::make_pair(std::string(name), symbol));
        return inserted.second;
    }

    SymbolBase* SchemaPoolBase::get_symbol(std::string_view full_name) {
        auto it = symbols_.find(std::string(full_name));
        if (it == symbols_.end()) {
            return nullptr;
        }
        return it->second;
    }

    const SymbolBase* SchemaPoolBase::get_symbol(std::string_view full_name) const {
        auto it = symbols_.find(std::string(full_name));
        if (it == symbols_.end()) {
            return nullptr;
        }
        return it->second;
    }

    bool SchemaPoolBase::add_file(FileBase* file) {
        if (file == nullptr || file->path_.empty()) {
            return false;
        }
        auto inserted = files_.insert(std::make_pair(file->path_, file));
        return inserted.second;
    }

    FileBase* SchemaPoolBase::get_file(std::string_view path) {
        auto it = files_.find(std::string(path));
        if (it == files_.end()) {
            return nullptr;
        }
        return it->second;
    }

    const FileBase* SchemaPoolBase::get_file(std::string_view path) const {
        auto it = files_.find(std::string(path));
        if (it == files_.end()) {
            return nullptr;
        }
        return it->second;
    }

    bool SchemaPoolBase::add_constrain(std::string_view name, constrain_func func) {
        if (name.empty() || !func) {
            return false;
        }
        auto inserted = constrains_.insert(std::make_pair(std::string(name), std::move(func)));
        return inserted.second;
    }

    constrain_func* SchemaPoolBase::get_constrain(std::string_view name) {
        auto it = constrains_.find(std::string(name));
        if (it == constrains_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    const constrain_func* SchemaPoolBase::get_constrain(std::string_view name) const {
        auto it = constrains_.find(std::string(name));
        if (it == constrains_.end()) {
            return nullptr;
        }
        return &it->second;
    }

}  // namespace shark
