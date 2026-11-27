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
#include <shark/parser/pool.h>
#include <shark/parser/struct.h>
#include <string>
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

        void export_symbol(SymbolNode* node, std::string_view name) {
            node->name = name;
            node->full_name_without_package.assign(name.data(), name.size());
            node->full_name = node->full_name_without_package;
            node->bind = true;
            node->emit_to_pool();
        }

        void add_parameter_field(RegistryPool& pool, StructParser* owner, std::string_view parameter) {
            auto* field = pool.create_symbol<FieldParser>(nullptr);
            field->container = owner;
            field->type_ref.name = parameter;
            field->type_ref.is_parameter = true;
            field->name = parameter;
            field->full_name_without_package = owner->full_name_without_package + "." + std::string(parameter);
            field->full_name = field->full_name_without_package;
            field->bind = true;
            field->emit_to_pool();
            owner->fields.push_back(field);

            TypeName argument;
            argument.name = parameter;
            argument.is_parameter = true;
            owner->declared_name.arguments.push_back(std::move(argument));
            owner->generic = true;
        }

        StructParser* add_container(RegistryPool& pool, std::string_view name, PhysicalType type) {
            if (SymbolNode* existing = pool.get_symbol(name)) {
                return static_cast<StructParser*>(existing);
            }
            auto* node = pool.create_symbol<StructParser>(nullptr);
            node->builtin = true;
            node->generic = true;
            node->phy_type = type;
            node->declared_name.name = name;
            export_symbol(node, name);
            return node;
        }

    }  // namespace

    bool load_builtin_types(RegistryPool& pool) {
        for (const BuiltinType& spec : kBuiltinTypes) {
            if (pool.get_symbol(spec.name) != nullptr) {
                continue;
            }
            auto* node = pool.create_symbol<StructParser>(nullptr);
            node->builtin = true;
            node->phy_type = spec.type;
            export_symbol(node, spec.name);
        }

        StructParser* list = add_container(pool, "list", PhysicalType::LIST);
        if (list->fields.empty()) {
            add_parameter_field(pool, list, "value");
        }
        StructParser* set = add_container(pool, "set", PhysicalType::SET);
        if (set->fields.empty()) {
            add_parameter_field(pool, set, "key");
        }
        StructParser* map = add_container(pool, "map", PhysicalType::MAP);
        if (map->fields.empty()) {
            add_parameter_field(pool, map, "key");
            add_parameter_field(pool, map, "value");
        }
        return true;
    }

}  // namespace shark::parser
