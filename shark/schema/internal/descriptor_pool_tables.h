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
#include <shark/schema/internal/flat_allocator.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

        // ===================================================================
    // DescriptorPool::Tables

    class DescriptorPool::Tables {
    public:
        Tables();
        ~Tables();

        // Record the current state of the tables to the stack of checkpoints.
        // Each call to AddCheckpoint() must be paired with exactly one call to either
        // ClearLastCheckpoint() or RollbackToLastCheckpoint().
        //
        // This is used when building files, since some kinds of validation errors
        // cannot be detected until the file's descriptors have already been added to
        // the tables.
        //
        // This supports recursive checkpoints, since building a file may trigger
        // recursive building of other files. Note that recursive checkpoints are not
        // normally necessary; explicit dependencies are built prior to checkpointing.
        // So although we recursively build transitive imports, there is at most one
        // checkpoint in the stack during dependency building.
        //
        // Recursive checkpoints only arise during cross-linking of the descriptors.
        // Symbol references must be resolved, via DescriptorBuilder::FindSymbol and
        // friends. If the pending file references an unknown symbol
        // (e.g., it is not defined in the pending file's explicit dependencies), and
        // the pool is using a fallback database, and that database contains a file
        // defining that symbol, and that file has not yet been built by the pool,
        // the pool builds the file during cross-linking, leading to another
        // checkpoint.
        void AddCheckpoint();

        // Mark the last checkpoint as having cleared successfully, removing it from
        // the stack. If the stack is empty, all pending symbols will be committed.
        //
        // Note that this does not guarantee that the symbols added since the last
        // checkpoint won't be rolled back: if a checkpoint gets rolled back,
        // everything past that point gets rolled back, including symbols added after
        // checkpoints that were pushed onto the stack after it and marked as cleared.
        void ClearLastCheckpoint();

        // Roll back the Tables to the state of the checkpoint at the top of the
        // stack, removing everything that was added after that point.
        void RollbackToLastCheckpoint();

        // The stack of files which are currently being built.  Used to detect
        // cyclic dependencies when loading files from a DescriptorDatabase.  Not
        // used when fallback_database_ == nullptr.
        std::vector<std::string> pending_files_;

        // A set of files which we have tried to load from the fallback database
        // and encountered errors.  We will not attempt to load them again during
        // execution of the current public API call, but for compatibility with
        // legacy clients, this is cleared at the beginning of each public API call.
        // Not used when fallback_database_ == nullptr.
        HASH_SET<std::string> known_bad_files_;

        // A set of symbols which we have tried to load from the fallback database
        // and encountered errors. We will not attempt to load them again during
        // execution of the current public API call, but for compatibility with
        // legacy clients, this is cleared at the beginning of each public API call.
        HASH_SET<std::string> known_bad_symbols_;

        // The set of descriptors for which we've already loaded the full
        // set of extensions numbers from fallback_database_.
        HASH_SET<const Descriptor*> extensions_loaded_from_db_;

        // Maps type name to Descriptor::WellKnownType.  This is logically global
        // and const, but we make it a member here to simplify its construction and
        // destruction.  This only has 20-ish entries and is one per DescriptorPool,
        // so the overhead is small.
        HASH_MAP<std::string, Descriptor::WellKnownType> well_known_types_;

        // -----------------------------------------------------------------
        // Finding items.

        // Find symbols.  This returns a null Symbol (symbol.IsNull() is true)
        // if not found.
        inline Symbol FindSymbol(StringPiece key) const;

        // This implements the body of DescriptorPool::Find*ByName().  It should
        // really be a private method of DescriptorPool, but that would require
        // declaring Symbol in descriptor.h, which would drag all kinds of other
        // stuff into the header.  Yay C++.
        Symbol FindByNameHelper(const DescriptorPool* pool, StringPiece name);

        // These return nullptr if not found.
        inline const FileDescriptor* FindFile(StringPiece key) const;
        inline const FieldDescriptor* FindExtension(const Descriptor* extendee,
            int number) const;
        inline void FindAllExtensions(const Descriptor* extendee,
            std::vector<const FieldDescriptor*>* out) const;

        // -----------------------------------------------------------------
        // Adding items.

        // These add items to the corresponding tables.  They return false if
        // the key already exists in the table.  For AddSymbol(), the string passed
        // in must be one that was constructed using AllocateString(), as it will
        // be used as a key in the symbols_by_name_ map without copying.
        bool AddSymbol(const std::string& full_name, Symbol symbol);
        bool AddFile(const FileDescriptor* file);
        bool AddExtension(const FieldDescriptor* field);

        // -----------------------------------------------------------------
        // Allocating memory.

        // Allocate an object which will be reclaimed when the pool is
        // destroyed.  Note that the object's destructor will never be called,
        // so its fields must be plain old data (primitive data types and
        // pointers).  All of the descriptor types are such objects.
        template <typename Type>
        Type* Allocate();

        // Allocate some bytes which will be reclaimed when the pool is
        // destroyed. Memory is aligned to 8 bytes.
        void* AllocateBytes(int size);

        // Create a FlatAllocation for the corresponding sizes.
        // All objects within it will be default constructed.
        // The whole allocation, including the non-trivial objects within, will be
        // destroyed with the pool.
        template <typename... T>
        internal::FlatAllocator::Allocation* CreateFlatAlloc(
            const TypeMap<IntT, T...>& sizes);

    private:
        // All memory allocated in the pool.  Must be first as other objects can
        // point into these.
        struct MiscDeleter {
            void operator()(int* p) const { internal::SizedDelete(p, *p + 8); }
        };
        // Miscellaneous allocations are length prefixed. The paylaod is 8 bytes after
        // the `int` that contains the size. This keeps the payload aligned.
        std::vector<std::unique_ptr<int, MiscDeleter>> misc_allocs_;
        struct FlatAllocDeleter {
            void operator()(internal::FlatAllocator::Allocation* p) const {
                p->Destroy();
            }
        };
        std::vector<
            std::unique_ptr<internal::FlatAllocator::Allocation, FlatAllocDeleter>>
            flat_allocs_;

        SymbolsByNameSet symbols_by_name_;
        FilesByNameMap files_by_name_;
        ExtensionsGroupedByDescriptorMap extensions_;

        struct CheckPoint {
            explicit CheckPoint(const Tables* tables)
                : flat_allocations_before_checkpoint(
                      static_cast<int>(tables->flat_allocs_.size()))
                , misc_allocations_before_checkpoint(
                      static_cast<int>(tables->misc_allocs_.size()))
                , pending_symbols_before_checkpoint(
                      tables->symbols_after_checkpoint_.size())
                , pending_files_before_checkpoint(
                      tables->files_after_checkpoint_.size())
                , pending_extensions_before_checkpoint(
                      tables->extensions_after_checkpoint_.size()) { }
            int flat_allocations_before_checkpoint;
            int misc_allocations_before_checkpoint;
            int pending_symbols_before_checkpoint;
            int pending_files_before_checkpoint;
            int pending_extensions_before_checkpoint;
        };
        std::vector<CheckPoint> checkpoints_;
        std::vector<Symbol> symbols_after_checkpoint_;
        std::vector<const FileDescriptor*> files_after_checkpoint_;
        std::vector<DescriptorIntPair> extensions_after_checkpoint_;
    };

    inline Symbol DescriptorPool::Tables::FindSymbol(StringPiece key) const {
        Symbol::QueryKey name;
        name.name = key;
        auto it = symbols_by_name_.find(name);
        return it == symbols_by_name_.end() ? Symbol() : *it;
    }

    inline const FileDescriptor* DescriptorPool::Tables::FindFile(
    StringPiece key) const {
        return FindPtrOrNull(files_by_name_, key);
    }

    inline const FieldDescriptor* DescriptorPool::Tables::FindExtension(
        const Descriptor* extendee, int number) const {
        return FindPtrOrNull(extensions_, std::make_pair(extendee, number));
    }

    inline void DescriptorPool::Tables::FindAllExtensions(
        const Descriptor* extendee,
        std::vector<const FieldDescriptor*>* out) const {
        ExtensionsGroupedByDescriptorMap::const_iterator it = extensions_.lower_bound(std::make_pair(extendee, 0));
        for (; it != extensions_.end() && it->first.first == extendee; ++it) {
            out->push_back(it->second);
        }
    }

    template <typename Type>
    Type* DescriptorPool::Tables::Allocate() {
        static_assert(std::is_trivially_destructible<Type>::value, "");
        static_assert(alignof(Type) <= 8, "");
        return ::new (AllocateBytes(sizeof(Type))) Type { };
    }

    template <typename... T>
internal::FlatAllocator::Allocation* DescriptorPool::Tables::CreateFlatAlloc(
    const TypeMap<IntT, T...>& sizes) {
        auto ends = CalculateEnds(sizes);
        using FlatAlloc = internal::FlatAllocator::Allocation;

        int last_end = ends.template Get<
            typename std::tuple_element<sizeof...(T) - 1, std::tuple<T...>>::type>();
        size_t total_size = last_end + RoundUpTo<FlatAlloc::kMaxAlign>(sizeof(FlatAlloc));
        char* data = static_cast<char*>(::operator new(total_size));
        auto* res = ::new (data) FlatAlloc(ends);
        flat_allocs_.emplace_back(res);

        return res;
    }
} // namespace shark


#include <shark/port_undef.inc>
