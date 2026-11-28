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

#include <shark/parser/alias.h>
#include <shark/parser/annotation.h>
#include <shark/parser/const_value.h>
#include <shark/parser/enum.h>
#include <shark/parser/enum_value.h>
#include <shark/parser/field.h>
#include <shark/parser/file.h>
#include <shark/parser/function.h>
#include <shark/parser/pool.h>
#include <shark/parser/primitive.h>
#include <shark/parser/servlet.h>
#include <shark/parser/struct.h>
#include <shark/parser/symbol.h>

#include <optional>

namespace shark::parser {

    namespace {

        const std::vector<AnnotationParser*>* own_annotations(SymbolNode* node) {
            switch (node->node_type) {
                case SymbolType::STRUCT:
                    return &static_cast<StructParser*>(node)->annotations;
                case SymbolType::PRIMITIVE:
                    return &static_cast<PrimitiveParser*>(node)->annotations;
                case SymbolType::ENUM:
                    return &static_cast<EnumParser*>(node)->annotations;
                case SymbolType::ENUM_VALUE:
                    return &static_cast<EnumValueParser*>(node)->annotations;
                case SymbolType::FIELD:
                    return &static_cast<FieldParser*>(node)->annotations;
                case SymbolType::FUNCTION:
                    return &static_cast<FunctionParser*>(node)->annotations;
                case SymbolType::SERVLET:
                    return &static_cast<ServletParser*>(node)->annotations;
                case SymbolType::ALIAS:
                    return &static_cast<AliasParser*>(node)->annotations;
                default:
                    return nullptr;
            }
        }

        std::optional<bool> export_flag(const std::vector<AnnotationParser*>* annotations, std::string_view key) {
            if (annotations == nullptr) {
                return std::nullopt;
            }
            for (AnnotationParser* annotation : *annotations) {
                if (annotation->classify != "export" || annotation->key != key || annotation->object == nullptr) {
                    continue;
                }
                if (const bool* value = std::get_if<bool>(&annotation->object->value)) {
                    return *value;
                }
            }
            return std::nullopt;
        }

        bool export_setting(SymbolNode* node, std::string_view key, bool fallback) {
            if (std::optional<bool> own = export_flag(own_annotations(node), key)) {
                return *own;
            }
            if (node->file != nullptr) {
                if (std::optional<bool> file_level = export_flag(&node->file->annotations, key)) {
                    return *file_level;
                }
            }
            return fallback;
        }

    }  // namespace

    bool SymbolNode::emit_to_pool() {
        const bool include_package = export_setting(this, "include_package", true);
        const bool exclude_package = export_setting(this, "exclude_package", false);
        if (include_package) {
            if (pool->get_symbol(full_name) != nullptr) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", full_name);
                return false;
            }
            if (!pool->add_symbol(this)) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", full_name);
                return false;
            }
        }
        if (exclude_package && full_name_without_package != full_name) {
            if (!pool->add_symbol_name(full_name_without_package, this)) {
                report_error(ErrorType::SEMANTIC, "duplicate symbol `%s`", full_name_without_package);
                return false;
            }
        }
        emit = true;
        return true;
    }
}  // namespace shark::parser

