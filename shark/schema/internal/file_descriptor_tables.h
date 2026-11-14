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
#include <shark/schema/internal/utils.h>
#include <shark/schema/internal/symbol.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // Contains tables specific to a particular file.  These tables are not
    // modified once the file has been constructed, so they need not be
    // protected by a mutex.  This makes operations that depend only on the
    // contents of a single file -- e.g. Descriptor::FindFieldByName() --
    // lock-free.
    //
    // For historical reasons, the definitions of the methods of
    // FileDescriptorTables and DescriptorPool::Tables are interleaved below.
    // These used to be a single class.
    class FileDescriptorTables {
    public:
        FileDescriptorTables();
        ~FileDescriptorTables();

        // Empty table, used with placeholder files.
        inline static const FileDescriptorTables& GetEmptyInstance();

        // -----------------------------------------------------------------
        // Finding items.

        // Returns a null Symbol (symbol.IsNull() is true) if not found.
        inline Symbol FindNestedSymbol(const void* parent,
            StringPiece name) const;

        // These return nullptr if not found.
        inline const FieldDescriptor* FindFieldByNumber(const Descriptor* parent,
            int number) const;
        inline const FieldDescriptor* FindFieldByLowercaseName(
            const void* parent, StringPiece lowercase_name) const;
        inline const FieldDescriptor* FindFieldByCamelcaseName(
            const void* parent, StringPiece camelcase_name) const;
        inline const EnumValueDescriptor* FindEnumValueByNumber(
            const EnumDescriptor* parent, int number) const;
        // This creates a new EnumValueDescriptor if not found, in a thread-safe way.
        inline const EnumValueDescriptor* FindEnumValueByNumberCreatingIfUnknown(
            const EnumDescriptor* parent, int number) const;

        // -----------------------------------------------------------------
        // Adding items.

        // These add items to the corresponding tables.  They return false if
        // the key already exists in the table.
        bool AddAliasUnderParent(const void* parent, const std::string& name,
            Symbol symbol);
        bool AddFieldByNumber(FieldDescriptor* field);
        bool AddEnumValueByNumber(EnumValueDescriptor* value);

        // Populates p->first->locations_by_path_ from p->second.
        // Unusual signature dictated by internal::call_once.
        static void BuildLocationsByPath(
            std::pair<const FileDescriptorTables*, const SourceCodeInfo*>* p);

        // Returns the location denoted by the specified path through info,
        // or nullptr if not found.
        // The value of info must be that of the corresponding FileDescriptor.
        // (Conceptually a pure function, but stateful as an optimisation.)
        const SourceCodeInfo_Location* GetSourceLocation(
            const std::vector<int>& path, const SourceCodeInfo* info) const;

        // Must be called after BuildFileImpl(), even if the build failed and
        // we are going to roll back to the last checkpoint.
        void FinalizeTables();

    private:
        const void* FindParentForFieldsByMap(const FieldDescriptor* field) const;
        static void FieldsByLowercaseNamesLazyInitStatic(
            const FileDescriptorTables* tables);
        void FieldsByLowercaseNamesLazyInitInternal() const;
        static void FieldsByCamelcaseNamesLazyInitStatic(
            const FileDescriptorTables* tables);
        void FieldsByCamelcaseNamesLazyInitInternal() const;

        SymbolsByParentSet symbols_by_parent_;
        mutable internal::once_flag fields_by_lowercase_name_once_;
        mutable internal::once_flag fields_by_camelcase_name_once_;
        // Make these fields atomic to avoid race conditions with
        // GetEstimatedOwnedMemoryBytesSize. Once the pointer is set the map won't
        // change anymore.
        mutable std::atomic<const FieldsByNameMap*> fields_by_lowercase_name_ { };
        mutable std::atomic<const FieldsByNameMap*> fields_by_camelcase_name_ { };
        FieldsByNumberSet fields_by_number_; // Not including extensions.
        EnumValuesByNumberSet enum_values_by_number_;
        mutable EnumValuesByNumberSet unknown_enum_values_by_number_
            SHARK_GUARDED_BY(unknown_enum_values_mu_);

        // Populated on first request to save space, hence constness games.
        mutable internal::once_flag locations_by_path_once_;
        mutable LocationsByPathMap locations_by_path_;

        // Mutex to protect the unknown-enum-value map due to dynamic
        // EnumValueDescriptor creation on unknown values.
        mutable internal::WrappedMutex unknown_enum_values_mu_;
    };

} // namespace shark


#include <shark/port_undef.inc>
