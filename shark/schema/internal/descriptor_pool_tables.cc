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
#include <shark/schema/internal/descriptor_pool_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    DescriptorPool::Tables::Tables() {
        well_known_types_.insert({
            { "shark.DoubleValue", Descriptor::WELLKNOWNTYPE_DOUBLEVALUE },
            { "shark.FloatValue", Descriptor::WELLKNOWNTYPE_FLOATVALUE },
            { "shark.Int64Value", Descriptor::WELLKNOWNTYPE_INT64VALUE },
            { "shark.UInt64Value", Descriptor::WELLKNOWNTYPE_UINT64VALUE },
            { "shark.Int32Value", Descriptor::WELLKNOWNTYPE_INT32VALUE },
            { "shark.UInt32Value", Descriptor::WELLKNOWNTYPE_UINT32VALUE },
            { "shark.StringValue", Descriptor::WELLKNOWNTYPE_STRINGVALUE },
            { "shark.BytesValue", Descriptor::WELLKNOWNTYPE_BYTESVALUE },
            { "shark.BoolValue", Descriptor::WELLKNOWNTYPE_BOOLVALUE },
            { "shark.Any", Descriptor::WELLKNOWNTYPE_ANY },
            { "shark.FieldMask", Descriptor::WELLKNOWNTYPE_FIELDMASK },
            { "shark.Duration", Descriptor::WELLKNOWNTYPE_DURATION },
            { "shark.Timestamp", Descriptor::WELLKNOWNTYPE_TIMESTAMP },
            { "shark.Value", Descriptor::WELLKNOWNTYPE_VALUE },
            { "shark.ListValue", Descriptor::WELLKNOWNTYPE_LISTVALUE },
            { "shark.Struct", Descriptor::WELLKNOWNTYPE_STRUCT },
        });
    }

    DescriptorPool::Tables::~Tables() {
        GOOGLE_DCHECK(checkpoints_.empty());
    }


    void DescriptorPool::Tables::AddCheckpoint() {
        checkpoints_.push_back(CheckPoint(this));
    }


    void DescriptorPool::Tables::ClearLastCheckpoint() {
        GOOGLE_DCHECK(!checkpoints_.empty());
        checkpoints_.pop_back();
        if (checkpoints_.empty()) {
            // All checkpoints have been cleared: we can now commit all of the pending
            // data.
            symbols_after_checkpoint_.clear();
            files_after_checkpoint_.clear();
            extensions_after_checkpoint_.clear();
        }
    }

    void DescriptorPool::Tables::RollbackToLastCheckpoint() {
        GOOGLE_DCHECK(!checkpoints_.empty());
        const CheckPoint& checkpoint = checkpoints_.back();

        for (size_t i = checkpoint.pending_symbols_before_checkpoint;
            i < symbols_after_checkpoint_.size(); i++) {
            symbols_by_name_.erase(symbols_after_checkpoint_[i]);
            }
        for (size_t i = checkpoint.pending_files_before_checkpoint;
            i < files_after_checkpoint_.size(); i++) {
            files_by_name_.erase(files_after_checkpoint_[i]->name());
            }
        for (size_t i = checkpoint.pending_extensions_before_checkpoint;
            i < extensions_after_checkpoint_.size(); i++) {
            extensions_.erase(extensions_after_checkpoint_[i]);
            }

        symbols_after_checkpoint_.resize(
            checkpoint.pending_symbols_before_checkpoint);
        files_after_checkpoint_.resize(checkpoint.pending_files_before_checkpoint);
        extensions_after_checkpoint_.resize(
            checkpoint.pending_extensions_before_checkpoint);

        flat_allocs_.resize(checkpoint.flat_allocations_before_checkpoint);
        misc_allocs_.resize(checkpoint.misc_allocations_before_checkpoint);
        checkpoints_.pop_back();
    }


    Symbol DescriptorPool::Tables::FindByNameHelper(const DescriptorPool* pool,
        StringPiece name) {
        if (pool->mutex_ != nullptr) {
            // Fast path: the Symbol is already cached.  This is just a hash lookup.
            ReaderMutexLock lock(pool->mutex_);
            if (known_bad_symbols_.empty() && known_bad_files_.empty()) {
                Symbol result = FindSymbol(name);
                if (!result.IsNull())
                    return result;
            }
        }
        MutexLockMaybe lock(pool->mutex_);
        if (pool->fallback_database_ != nullptr) {
            known_bad_symbols_.clear();
            known_bad_files_.clear();
        }
        Symbol result = FindSymbol(name);

        if (result.IsNull() && pool->underlay_ != nullptr) {
            // Symbol not found; check the underlay.
            result = pool->underlay_->tables_->FindByNameHelper(pool->underlay_, name);
        }

        if (result.IsNull()) {
            // Symbol still not found, so check fallback database.
            if (pool->TryFindSymbolInFallbackDatabase(name)) {
                result = FindSymbol(name);
            }
        }

        return result;
    }

    bool DescriptorPool::Tables::AddSymbol(const std::string& full_name,
        Symbol symbol) {
        GOOGLE_DCHECK_EQ(full_name, symbol.full_name());
        if (symbols_by_name_.insert(symbol).second) {
            symbols_after_checkpoint_.push_back(symbol);
            return true;
        } else {
            return false;
        }
    }


    bool DescriptorPool::Tables::AddFile(const FileDescriptor* file) {
        if (InsertIfNotPresent(&files_by_name_, file->name(), file)) {
            files_after_checkpoint_.push_back(file);
            return true;
        } else {
            return false;
        }
    }

    bool DescriptorPool::Tables::AddExtension(const FieldDescriptor* field) {
        DescriptorIntPair key(field->containing_type(), field->number());
        if (InsertIfNotPresent(&extensions_, key, field)) {
            extensions_after_checkpoint_.push_back(key);
            return true;
        } else {
            return false;
        }
    }

    void* DescriptorPool::Tables::AllocateBytes(int size) {
        if (size == 0)
            return nullptr;
        void* p = ::operator new(size + RoundUpTo<8>(sizeof(int)));
        int* sizep = static_cast<int*>(p);
        misc_allocs_.emplace_back(sizep);
        *sizep = size;
        return static_cast<char*>(p) + RoundUpTo<8>(sizeof(int));
    }


} // namespace shark


#include <shark/port_undef.inc>
