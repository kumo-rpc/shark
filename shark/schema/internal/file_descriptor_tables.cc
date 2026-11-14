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

#include <cstdint>

#include <shark/schema/internal/file_descriptor_tables.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    FileDescriptorTables::FileDescriptorTables() { }

    FileDescriptorTables::~FileDescriptorTables() {
        delete fields_by_lowercase_name_.load(std::memory_order_acquire);
        delete fields_by_camelcase_name_.load(std::memory_order_acquire);
    }





    const void* FileDescriptorTables::FindParentForFieldsByMap(
        const FieldDescriptor* field) const {
        if (field->is_extension()) {
            if (field->extension_scope() == nullptr) {
                return field->file();
            } else {
                return field->extension_scope();
            }
        } else {
            return field->containing_type();
        }
    }

    void FileDescriptorTables::FieldsByLowercaseNamesLazyInitStatic(
        const FileDescriptorTables* tables) {
        tables->FieldsByLowercaseNamesLazyInitInternal();
    }

    void FileDescriptorTables::FieldsByLowercaseNamesLazyInitInternal() const {
        auto* map = new FieldsByNameMap;
        for (Symbol symbol : symbols_by_parent_) {
            const FieldDescriptor* field = symbol.field_descriptor();
            if (!field)
                continue;
            (*map)[{ FindParentForFieldsByMap(field), field->lowercase_name().c_str() }] = field;
        }
        fields_by_lowercase_name_.store(map, std::memory_order_release);
    }

    void FileDescriptorTables::FieldsByCamelcaseNamesLazyInitStatic(
        const FileDescriptorTables* tables) {
        tables->FieldsByCamelcaseNamesLazyInitInternal();
    }

    void FileDescriptorTables::FieldsByCamelcaseNamesLazyInitInternal() const {
        auto* map = new FieldsByNameMap;
        for (Symbol symbol : symbols_by_parent_) {
            const FieldDescriptor* field = symbol.field_descriptor();
            if (!field)
                continue;
            (*map)[{ FindParentForFieldsByMap(field), field->camelcase_name().c_str() }] = field;
        }
        fields_by_camelcase_name_.store(map, std::memory_order_release);
    }

    bool FileDescriptorTables::AddAliasUnderParent(const void* parent,
       const std::string& name,
       Symbol symbol) {
        GOOGLE_DCHECK_EQ(name, symbol.parent_name_key().second);
        GOOGLE_DCHECK_EQ(parent, symbol.parent_name_key().first);
        return symbols_by_parent_.insert(symbol).second;
    }

    void FileDescriptorTables::FinalizeTables() { }

    bool FileDescriptorTables::AddFieldByNumber(FieldDescriptor* field) {
        // Skip fields that are at the start of the sequence.
        if (field->containing_type() != nullptr && field->number() >= 1 && field->number() <= field->containing_type()->sequential_field_limit_) {
            if (field->is_extension()) {
                // Conflicts with the field that already exists in the sequential range.
                return false;
            }
            // Only return true if the field at that index matches. Otherwise it
            // conflicts with the existing field in the sequential range.
            return field->containing_type()->field(field->number() - 1) == field;
        }

        return fields_by_number_.insert(Symbol(field)).second;
    }

    bool FileDescriptorTables::AddEnumValueByNumber(EnumValueDescriptor* value) {
        // Skip values that are at the start of the sequence.
        const int base = value->type()->value(0)->number();
        if (base <= value->number() && value->number() <= static_cast<int64_t>(base) + value->type()->sequential_value_limit_)
            return true;
        return enum_values_by_number_.insert(Symbol::EnumValue(value, 0)).second;
    }

    void FileDescriptorTables::BuildLocationsByPath(
        std::pair<const FileDescriptorTables*, const SourceCodeInfo*>* p) {
        for (int i = 0, len = p->second->location_size(); i < len; ++i) {
            const SourceCodeInfo_Location* loc = &p->second->location().Get(i);
            p->first->locations_by_path_[Join(loc->path(), ",")] = loc;
        }
    }

    const SourceCodeInfo_Location* FileDescriptorTables::GetSourceLocation(
        const std::vector<int>& path, const SourceCodeInfo* info) const {
        std::pair<const FileDescriptorTables*, const SourceCodeInfo*> p(
            std::make_pair(this, info));
        internal::call_once(locations_by_path_once_,
            FileDescriptorTables::BuildLocationsByPath, &p);
        return FindPtrOrNull(locations_by_path_, Join(path, ","));
    }

} // namespace shark


#include <shark/port_undef.inc>
