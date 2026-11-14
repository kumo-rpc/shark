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

#include <cstdint>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/descriptor_pool_tables.h>
#include <shark/schema/internal/flat_allocator.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    inline const FileDescriptorTables& FileDescriptorTables::GetEmptyInstance() {
        static auto file_descriptor_tables = internal::OnShutdownDelete(new FileDescriptorTables());
        return *file_descriptor_tables;
    }

    inline Symbol FileDescriptorTables::FindNestedSymbol(
    const void* parent, StringPiece name) const {
        Symbol::QueryKey query;
        query.name = name;
        query.parent = parent;
        auto it = symbols_by_parent_.find(query);
        return it == symbols_by_parent_.end() ? Symbol() : *it;
    }


    inline const FieldDescriptor* FileDescriptorTables::FindFieldByNumber(
        const Descriptor* parent, int number) const {
        // If `number` is within the sequential range, just index into the parent
        // without doing a table lookup.
        if (parent != nullptr && //
            1 <= number && number <= parent->sequential_field_limit_) {
            return parent->field(number - 1);
            }

        Symbol::QueryKey query;
        query.parent = parent;
        query.field_number = number;

        auto it = fields_by_number_.find(query);
        return it == fields_by_number_.end() ? nullptr : it->field_descriptor();
    }

    inline const FieldDescriptor* FileDescriptorTables::FindFieldByLowercaseName(
    const void* parent, StringPiece lowercase_name) const {
        internal::call_once(
            fields_by_lowercase_name_once_,
            &FileDescriptorTables::FieldsByLowercaseNamesLazyInitStatic, this);
        return FindPtrOrNull(
            *fields_by_lowercase_name_.load(std::memory_order_acquire),
            PointerStringPair(parent, lowercase_name));
    }

    inline const FieldDescriptor* FileDescriptorTables::FindFieldByCamelcaseName(
        const void* parent, StringPiece camelcase_name) const {
        internal::call_once(
            fields_by_camelcase_name_once_,
            FileDescriptorTables::FieldsByCamelcaseNamesLazyInitStatic, this);
        return FindPtrOrNull(
            *fields_by_camelcase_name_.load(std::memory_order_acquire),
            PointerStringPair(parent, camelcase_name));
    }

    inline const EnumValueDescriptor* FileDescriptorTables::FindEnumValueByNumber(
        const EnumDescriptor* parent, int number) const {
        // If `number` is within the sequential range, just index into the parent
        // without doing a table lookup.
        const int base = parent->value(0)->number();
        if (base <= number && number <= static_cast<int64_t>(base) + parent->sequential_value_limit_) {
            return parent->value(number - base);
        }

        Symbol::QueryKey query;
        query.parent = parent;
        query.field_number = number;

        auto it = enum_values_by_number_.find(query);
        return it == enum_values_by_number_.end() ? nullptr
                                                  : it->enum_value_descriptor();
    }

    inline const EnumValueDescriptor*
    FileDescriptorTables::FindEnumValueByNumberCreatingIfUnknown(
        const EnumDescriptor* parent, int number) const {
        // First try, with map of compiled-in values.
        {
            const auto* value = FindEnumValueByNumber(parent, number);
            if (value != nullptr) {
                return value;
            }
        }

        Symbol::QueryKey query;
        query.parent = parent;
        query.field_number = number;

        // Second try, with reader lock held on unknown enum values: common case.
        {
            ReaderMutexLock l(&unknown_enum_values_mu_);
            auto it = unknown_enum_values_by_number_.find(query);
            if (it != unknown_enum_values_by_number_.end() && it->enum_value_descriptor() != nullptr) {
                return it->enum_value_descriptor();
            }
        }
        // If not found, try again with writer lock held, and create new descriptor if
        // necessary.
        {
            WriterMutexLock l(&unknown_enum_values_mu_);
            auto it = unknown_enum_values_by_number_.find(query);
            if (it != unknown_enum_values_by_number_.end() && it->enum_value_descriptor() != nullptr) {
                return it->enum_value_descriptor();
            }

            // Create an EnumValueDescriptor dynamically. We don't insert it into the
            // EnumDescriptor (it's not a part of the enum as originally defined), but
            // we do insert it into the table so that we can return the same pointer
            // later.
            std::string enum_value_name = StringPrintf(
                "UNKNOWN_ENUM_VALUE_%s_%d", parent->name().c_str(), number);
            auto* pool = DescriptorPool::generated_pool();
            auto* tables = const_cast<DescriptorPool::Tables*>(pool->tables_.get());
            internal::FlatAllocator alloc;
            alloc.PlanArray<EnumValueDescriptor>(1);
            alloc.PlanArray<std::string>(2);

            {
                // Must lock the pool because we will do allocations in the shared arena.
                MutexLockMaybe l2(pool->mutex_);
                alloc.FinalizePlanning(tables);
            }
            EnumValueDescriptor* result = alloc.AllocateArray<EnumValueDescriptor>(1);
            result->all_names_ = alloc.AllocateStrings(
                enum_value_name,
                StrCat(parent->full_name(), ".", enum_value_name));
            result->number_ = number;
            result->type_ = parent;
            result->options_ = &EnumValueOptions::default_instance();
            unknown_enum_values_by_number_.insert(Symbol::EnumValue(result, 0));
            return result;
        }
    }
} // namespace shark


#include <shark/port_undef.inc>
