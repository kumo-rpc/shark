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
#include <shark/parser/field.h>
#include <shark/parser/placeholder.h>
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>
#include <string_view>

namespace shark::parser {

    namespace {

        struct BuiltinType {
            std::string_view name;
            PhysicalType type;
        };

        constexpr BuiltinType kBuiltinTypes[] = {
            {"bool", PhysicalType::BOOL},
            {"int8", PhysicalType::INT8},
            {"uint8", PhysicalType::UINT8},
            {"int16", PhysicalType::INT16},
            {"uint16", PhysicalType::UINT16},
            {"int32", PhysicalType::INT32},
            {"uint32", PhysicalType::UINT32},
            {"int64", PhysicalType::INT64},
            {"uint64", PhysicalType::UINT64},
            {"int128", PhysicalType::INT128},
            {"uint128", PhysicalType::UINT128},
            {"float32", PhysicalType::FLOAT32},
            {"float64", PhysicalType::FLOAT64},
            {"string", PhysicalType::STRING},
        };

        constexpr PhysicalType kKeyTypes[] = {
            PhysicalType::BOOL,
            PhysicalType::INT8,
            PhysicalType::UINT8,
            PhysicalType::INT16,
            PhysicalType::UINT16,
            PhysicalType::INT32,
            PhysicalType::UINT32,
            PhysicalType::INT64,
            PhysicalType::UINT64,
            PhysicalType::INT128,
            PhysicalType::UINT128,
            PhysicalType::STRING,
        };

        void export_symbol(SymbolNode* node, std::string_view name) {
            node->name = name;
            node->full_name_without_package.assign(name.data(), name.size());
            node->full_name = node->full_name_without_package;
            node->bind = true;
            node->emit_to_pool();
        }

        PlaceholderNode* add_placeholder(RegistryPool& pool, std::string_view name, PlaceholderNode::Constraint constraint, const PhysicalType* allowed, std::size_t allowed_count) {
            if (SymbolNode* existing = pool.get_symbol(name)) {
                return static_cast<PlaceholderNode*>(existing);
            }
            auto* node = pool.create_symbol<PlaceholderNode>(nullptr);
            node->constraint = constraint;
            node->allowed = allowed;
            node->allowed_count = allowed_count;
            export_symbol(node, name);
            return node;
        }

        void add_placeholder_field(RegistryPool& pool, StructNode* owner, PlaceholderNode* placeholder) {
            auto* field = pool.create_symbol<FieldNode>(nullptr);
            field->container = owner;
            field->placeholder = placeholder;
            field->type_ref = placeholder->name;
            field->name = placeholder->name;
            field->full_name_without_package = owner->full_name_without_package + "." + std::string(placeholder->name);
            field->full_name = field->full_name_without_package;
            field->bind = true;
            field->emit_to_pool();
            owner->fields.push_back(field);
        }

        StructNode* add_container(RegistryPool& pool, std::string_view name, PhysicalType type) {
            if (SymbolNode* existing = pool.get_symbol(name)) {
                return static_cast<StructNode*>(existing);
            }
            auto* node = pool.create_symbol<StructNode>(nullptr);
            node->builtin = true;
            node->generic = true;
            node->phy_type = type;
            export_symbol(node, name);
            return node;
        }

    }  // namespace

    bool load_builtin_types(RegistryPool& pool) {
        for (const BuiltinType& spec : kBuiltinTypes) {
            if (pool.get_symbol(spec.name) != nullptr) {
                continue;
            }
            auto* node = pool.create_symbol<StructNode>(nullptr);
            node->builtin = true;
            node->phy_type = spec.type;
            export_symbol(node, spec.name);
        }

        PlaceholderNode* key = add_placeholder(pool, "key", PlaceholderNode::Constraint::KEY, kKeyTypes, sizeof(kKeyTypes) / sizeof(kKeyTypes[0]));
        PlaceholderNode* value = add_placeholder(pool, "value", PlaceholderNode::Constraint::ALL, nullptr, 0);

        StructNode* list = add_container(pool, "list", PhysicalType::LIST);
        if (list->fields.empty()) {
            add_placeholder_field(pool, list, value);
        }
        StructNode* set = add_container(pool, "set", PhysicalType::SET);
        if (set->fields.empty()) {
            add_placeholder_field(pool, set, key);
        }
        StructNode* map = add_container(pool, "map", PhysicalType::MAP);
        if (map->fields.empty()) {
            add_placeholder_field(pool, map, key);
            add_placeholder_field(pool, map, value);
        }
        return true;
    }

}  // namespace shark::parser
