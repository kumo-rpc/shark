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

#include <shark/schema/internal/descriptor_pool.h>
#include <shark/schema/internal/descriptor_pool_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>
#include <shark/schema/internal/flat_allocator.h>
#include <shark/schema/internal/descriptor_builder.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    DescriptorPool::ErrorCollector::~ErrorCollector() { }

    DescriptorPool::DescriptorPool()
        : mutex_(nullptr)
        , fallback_database_(nullptr)
        , default_error_collector_(nullptr)
        , underlay_(nullptr)
        , tables_(new Tables)
        , enforce_dependencies_(true)
        , lazily_build_dependencies_(false)
        , allow_unknown_(false)
        , enforce_weak_(false)
        , disallow_enforce_utf8_(false) { }

    DescriptorPool::DescriptorPool(DescriptorDatabase* fallback_database,
        ErrorCollector* error_collector)
        : mutex_(new internal::WrappedMutex)
        , fallback_database_(fallback_database)
        , default_error_collector_(error_collector)
        , underlay_(nullptr)
        , tables_(new Tables)
        , enforce_dependencies_(true)
        , lazily_build_dependencies_(false)
        , allow_unknown_(false)
        , enforce_weak_(false)
        , disallow_enforce_utf8_(false) { }

    DescriptorPool::DescriptorPool(const DescriptorPool* underlay)
        : mutex_(nullptr)
        , fallback_database_(nullptr)
        , default_error_collector_(nullptr)
        , underlay_(underlay)
        , tables_(new Tables)
        , enforce_dependencies_(true)
        , lazily_build_dependencies_(false)
        , allow_unknown_(false)
        , enforce_weak_(false)
        , disallow_enforce_utf8_(false) { }

    DescriptorPool::~DescriptorPool() {
        if (mutex_ != nullptr)
            delete mutex_;
    }

    // DescriptorPool::BuildFile() defined later.
    // DescriptorPool::BuildFileCollectingErrors() defined later.

    void DescriptorPool::InternalDontEnforceDependencies() {
        enforce_dependencies_ = false;
    }

    void DescriptorPool::AddUnusedImportTrackFile(ConstStringParam file_name,
        bool is_error) {
        unused_import_track_files_[std::string(file_name)] = is_error;
    }

    void DescriptorPool::ClearUnusedImportTrackFiles() {
        unused_import_track_files_.clear();
    }

    bool DescriptorPool::InternalIsFileLoaded(ConstStringParam filename) const {
        MutexLockMaybe lock(mutex_);
        return tables_->FindFile(filename) != nullptr;
    }


    namespace {

        EncodedDescriptorDatabase* GeneratedDatabase() {
            static auto generated_database = internal::OnShutdownDelete(new EncodedDescriptorDatabase());
            return generated_database;
        }

        DescriptorPool* NewGeneratedPool() {
            auto generated_pool = new DescriptorPool(GeneratedDatabase());
            generated_pool->InternalSetLazilyBuildDependencies();
            return generated_pool;
        }

    } // anonymous namespace

    DescriptorDatabase* DescriptorPool::internal_generated_database() {
        return GeneratedDatabase();
    }


    DescriptorPool* DescriptorPool::internal_generated_pool() {
        static DescriptorPool* generated_pool = internal::OnShutdownDelete(NewGeneratedPool());
        return generated_pool;
    }

    const DescriptorPool* DescriptorPool::generated_pool() {
        const DescriptorPool* pool = internal_generated_pool();
        // Ensure that descriptor.shark has been registered in the generated pool.
        DescriptorProto::descriptor();
        return pool;
    }

    void DescriptorPool::InternalAddGeneratedFile(
        const void* encoded_file_descriptor, int size) {
        // So, this function is called in the process of initializing the
        // descriptors for generated proto classes.  Each generated .skp.cc file
        // has an internal procedure called AddDescriptors() which is called at
        // process startup, and that function calls this one in order to register
        // the raw bytes of the FileDescriptorProto representing the file.
        //
        // We do not actually construct the descriptor objects right away.  We just
        // hang on to the bytes until they are actually needed.  We actually construct
        // the descriptor the first time one of the following things happens:
        // * Someone calls a method like descriptor(), GetDescriptor(), or
        //   GetReflection() on the generated types, which requires returning the
        //   descriptor or an object based on it.
        // * Someone looks up the descriptor in DescriptorPool::generated_pool().
        //
        // Once one of these happens, the DescriptorPool actually parses the
        // FileDescriptorProto and generates a FileDescriptor (and all its children)
        // based on it.
        //
        // Note that FileDescriptorProto is itself a generated protocol message.
        // Therefore, when we parse one, we have to be very careful to avoid using
        // any descriptor-based operations, since this might cause infinite recursion
        // or deadlock.
        GOOGLE_CHECK(GeneratedDatabase()->Add(encoded_file_descriptor, size));
    }

    // Find*By* methods ==================================================

    // TODO(kenton):  There's a lot of repeated code here, but I'm not sure if
    //   there's any good way to factor it out.  Think about this some time when
    //   there's nothing more important to do (read: never).

    const FileDescriptor* DescriptorPool::FindFileByName(
        ConstStringParam name) const {
        MutexLockMaybe lock(mutex_);
        if (fallback_database_ != nullptr) {
            tables_->known_bad_symbols_.clear();
            tables_->known_bad_files_.clear();
        }
        const FileDescriptor* result = tables_->FindFile(name);
        if (result != nullptr)
            return result;
        if (underlay_ != nullptr) {
            result = underlay_->FindFileByName(name);
            if (result != nullptr)
                return result;
        }
        if (TryFindFileInFallbackDatabase(name)) {
            result = tables_->FindFile(name);
            if (result != nullptr)
                return result;
        }
        return nullptr;
    }

    const FileDescriptor* DescriptorPool::FindFileContainingSymbol(
        ConstStringParam symbol_name) const {
        MutexLockMaybe lock(mutex_);
        if (fallback_database_ != nullptr) {
            tables_->known_bad_symbols_.clear();
            tables_->known_bad_files_.clear();
        }
        Symbol result = tables_->FindSymbol(symbol_name);
        if (!result.IsNull())
            return result.GetFile();
        if (underlay_ != nullptr) {
            const FileDescriptor* file_result = underlay_->FindFileContainingSymbol(symbol_name);
            if (file_result != nullptr)
                return file_result;
        }
        if (TryFindSymbolInFallbackDatabase(symbol_name)) {
            result = tables_->FindSymbol(symbol_name);
            if (!result.IsNull())
                return result.GetFile();
        }
        return nullptr;
    }

    const Descriptor* DescriptorPool::FindMessageTypeByName(
        ConstStringParam name) const {
        return tables_->FindByNameHelper(this, name).descriptor();
    }

    const FieldDescriptor* DescriptorPool::FindFieldByName(
        ConstStringParam name) const {
        if (const FieldDescriptor* field = tables_->FindByNameHelper(this, name).field_descriptor()) {
            if (!field->is_extension()) {
                return field;
            }
        }
        return nullptr;
    }

    const FieldDescriptor* DescriptorPool::FindExtensionByName(
        ConstStringParam name) const {
        if (const FieldDescriptor* field = tables_->FindByNameHelper(this, name).field_descriptor()) {
            if (field->is_extension()) {
                return field;
            }
        }
        return nullptr;
    }

    const OneofDescriptor* DescriptorPool::FindOneofByName(
        ConstStringParam name) const {
        return tables_->FindByNameHelper(this, name).oneof_descriptor();
    }

    const EnumDescriptor* DescriptorPool::FindEnumTypeByName(
        ConstStringParam name) const {
        return tables_->FindByNameHelper(this, name).enum_descriptor();
    }

    const EnumValueDescriptor* DescriptorPool::FindEnumValueByName(
        ConstStringParam name) const {
        return tables_->FindByNameHelper(this, name).enum_value_descriptor();
    }

    const ServiceDescriptor* DescriptorPool::FindServiceByName(
        ConstStringParam name) const {
        return tables_->FindByNameHelper(this, name).service_descriptor();
    }

    const MethodDescriptor* DescriptorPool::FindMethodByName(
        ConstStringParam name) const {
        return tables_->FindByNameHelper(this, name).method_descriptor();
    }

    const FieldDescriptor* DescriptorPool::FindExtensionByNumber(
        const Descriptor* extendee, int number) const {
        if (extendee->extension_range_count() == 0)
            return nullptr;
        // A faster path to reduce lock contention in finding extensions, assuming
        // most extensions will be cache hit.
        if (mutex_ != nullptr) {
            ReaderMutexLock lock(mutex_);
            const FieldDescriptor* result = tables_->FindExtension(extendee, number);
            if (result != nullptr) {
                return result;
            }
        }
        MutexLockMaybe lock(mutex_);
        if (fallback_database_ != nullptr) {
            tables_->known_bad_symbols_.clear();
            tables_->known_bad_files_.clear();
        }
        const FieldDescriptor* result = tables_->FindExtension(extendee, number);
        if (result != nullptr) {
            return result;
        }
        if (underlay_ != nullptr) {
            result = underlay_->FindExtensionByNumber(extendee, number);
            if (result != nullptr)
                return result;
        }
        if (TryFindExtensionInFallbackDatabase(extendee, number)) {
            result = tables_->FindExtension(extendee, number);
            if (result != nullptr) {
                return result;
            }
        }
        return nullptr;
    }

    const FieldDescriptor* DescriptorPool::InternalFindExtensionByNumberNoLock(
        const Descriptor* extendee, int number) const {
        if (extendee->extension_range_count() == 0)
            return nullptr;

        const FieldDescriptor* result = tables_->FindExtension(extendee, number);
        if (result != nullptr) {
            return result;
        }

        if (underlay_ != nullptr) {
            result = underlay_->InternalFindExtensionByNumberNoLock(extendee, number);
            if (result != nullptr)
                return result;
        }

        return nullptr;
    }

    const FieldDescriptor* DescriptorPool::FindExtensionByPrintableName(
        const Descriptor* extendee, ConstStringParam printable_name) const {
        if (extendee->extension_range_count() == 0)
            return nullptr;
        const FieldDescriptor* result = FindExtensionByName(printable_name);
        if (result != nullptr && result->containing_type() == extendee) {
            return result;
        }
        if (extendee->options().message_set_wire_format()) {
            // MessageSet extensions may be identified by type name.
            const Descriptor* type = FindMessageTypeByName(printable_name);
            if (type != nullptr) {
                // Look for a matching extension in the foreign type's scope.
                const int type_extension_count = type->extension_count();
                for (int i = 0; i < type_extension_count; i++) {
                    const FieldDescriptor* extension = type->extension(i);
                    if (extension->containing_type() == extendee && extension->type() == SharkFieldType::MESSAGE && extension->is_optional() && extension->message_type() == type) {
                        // Found it.
                        return extension;
                    }
                }
            }
        }
        return nullptr;
    }

    void DescriptorPool::FindAllExtensions(
        const Descriptor* extendee,
        std::vector<const FieldDescriptor*>* out) const {
        MutexLockMaybe lock(mutex_);
        if (fallback_database_ != nullptr) {
            tables_->known_bad_symbols_.clear();
            tables_->known_bad_files_.clear();
        }

        // Initialize tables_->extensions_ from the fallback database first
        // (but do this only once per descriptor).
        if (fallback_database_ != nullptr && tables_->extensions_loaded_from_db_.count(extendee) == 0) {
            std::vector<int> numbers;
            if (fallback_database_->FindAllExtensionNumbers(extendee->full_name(),
                    &numbers)) {
                for (int number : numbers) {
                    if (tables_->FindExtension(extendee, number) == nullptr) {
                        TryFindExtensionInFallbackDatabase(extendee, number);
                    }
                }
                tables_->extensions_loaded_from_db_.insert(extendee);
            }
        }

        tables_->FindAllExtensions(extendee, out);
        if (underlay_ != nullptr) {
            underlay_->FindAllExtensions(extendee, out);
        }
    }

    bool DescriptorPool::TryFindFileInFallbackDatabase(
        StringPiece name) const {
        if (fallback_database_ == nullptr)
            return false;

        auto name_string = std::string(name);
        if (tables_->known_bad_files_.count(name_string) > 0)
            return false;

        FileDescriptorProto file_proto;
        if (!fallback_database_->FindFileByName(name_string, &file_proto) || BuildFileFromDatabase(file_proto) == nullptr) {
            tables_->known_bad_files_.insert(std::move(name_string));
            return false;
        }
        return true;
    }

    bool DescriptorPool::IsSubSymbolOfBuiltType(StringPiece name) const {
        auto prefix = std::string(name);
        for (;;) {
            std::string::size_type dot_pos = prefix.find_last_of('.');
            if (dot_pos == std::string::npos) {
                break;
            }
            prefix = prefix.substr(0, dot_pos);
            Symbol symbol = tables_->FindSymbol(prefix);
            // If the symbol type is anything other than PACKAGE, then its complete
            // definition is already known.
            if (!symbol.IsNull() && !symbol.IsPackage()) {
                return true;
            }
        }
        if (underlay_ != nullptr) {
            // Check to see if any prefix of this symbol exists in the underlay.
            return underlay_->IsSubSymbolOfBuiltType(name);
        }
        return false;
    }

    bool DescriptorPool::TryFindSymbolInFallbackDatabase(
        StringPiece name) const {
        if (fallback_database_ == nullptr)
            return false;

        auto name_string = std::string(name);
        if (tables_->known_bad_symbols_.count(name_string) > 0)
            return false;

        FileDescriptorProto file_proto;
        if ( // We skip looking in the fallback database if the name is a sub-symbol
             // of any descriptor that already exists in the descriptor pool (except
             // for package descriptors).  This is valid because all symbols except
             // for packages are defined in a single file, so if the symbol exists
             // then we should already have its definition.
             //
             // The other reason to do this is to support "overriding" type
             // definitions by merging two databases that define the same type. (Yes,
             // people do this.)  The main difficulty with making this work is that
             // FindFileContainingSymbol() is allowed to return both false positives
             // (e.g., SimpleDescriptorDatabase, UpgradedDescriptorDatabase) and
             // false negatives (e.g. ProtoFileParser, SourceTreeDescriptorDatabase).
             // When two such databases are merged, looking up a non-existent
             // sub-symbol of a type that already exists in the descriptor pool can
             // result in an attempt to load multiple definitions of the same type.
             // The check below avoids this.
            IsSubSymbolOfBuiltType(name)

            // Look up file containing this symbol in fallback database.
            || !fallback_database_->FindFileContainingSymbol(name_string, &file_proto)

            // Check if we've already built this file. If so, it apparently doesn't
            // contain the symbol we're looking for.  Some DescriptorDatabases
            // return false positives.
            || tables_->FindFile(file_proto.name()) != nullptr

            // Build the file.
            || BuildFileFromDatabase(file_proto) == nullptr) {
            tables_->known_bad_symbols_.insert(std::move(name_string));
            return false;
        }

        return true;
    }

    bool DescriptorPool::TryFindExtensionInFallbackDatabase(
        const Descriptor* containing_type, int field_number) const {
        if (fallback_database_ == nullptr)
            return false;

        FileDescriptorProto file_proto;
        if (!fallback_database_->FindFileContainingExtension(
                containing_type->full_name(), field_number, &file_proto)) {
            return false;
        }

        if (tables_->FindFile(file_proto.name()) != nullptr) {
            // We've already loaded this file, and it apparently doesn't contain the
            // extension we're looking for.  Some DescriptorDatabases return false
            // positives.
            return false;
        }

        if (BuildFileFromDatabase(file_proto) == nullptr) {
            return false;
        }

        return true;
    }

    void Descriptor::GetLocationPath(std::vector<int>* output) const {
        if (containing_type()) {
            containing_type()->GetLocationPath(output);
            output->push_back(DescriptorProto::kNestedTypeFieldNumber);
            output->push_back(index());
        } else {
            output->push_back(FileDescriptorProto::kMessageTypeFieldNumber);
            output->push_back(index());
        }
    }

    const FileDescriptor* DescriptorPool::BuildFile(
        const FileDescriptorProto& proto) {
        GOOGLE_CHECK(fallback_database_ == nullptr)
            << "Cannot call BuildFile on a DescriptorPool that uses a "
               "DescriptorDatabase.  You must instead find a way to get your file "
               "into the underlying database.";
        GOOGLE_CHECK(mutex_ == nullptr); // Implied by the above GOOGLE_CHECK.
        tables_->known_bad_symbols_.clear();
        tables_->known_bad_files_.clear();
        return DescriptorBuilder(this, tables_.get(), nullptr).BuildFile(proto);
    }


    const FileDescriptor* DescriptorPool::BuildFileCollectingErrors(
    const FileDescriptorProto& proto, ErrorCollector* error_collector) {
        GOOGLE_CHECK(fallback_database_ == nullptr)
            << "Cannot call BuildFile on a DescriptorPool that uses a "
               "DescriptorDatabase.  You must instead find a way to get your file "
               "into the underlying database.";
        GOOGLE_CHECK(mutex_ == nullptr); // Implied by the above GOOGLE_CHECK.
        tables_->known_bad_symbols_.clear();
        tables_->known_bad_files_.clear();
        return DescriptorBuilder(this, tables_.get(), error_collector)
            .BuildFile(proto);
    }

    const FileDescriptor* DescriptorPool::BuildFileFromDatabase(
        const FileDescriptorProto& proto) const {
        mutex_->AssertHeld();
        if (tables_->known_bad_files_.count(proto.name()) > 0) {
            return nullptr;
        }
        const FileDescriptor* result = DescriptorBuilder(this, tables_.get(), default_error_collector_)
                                           .BuildFile(proto);
        if (result == nullptr) {
            tables_->known_bad_files_.insert(proto.name());
        }
        return result;
    }

    Symbol DescriptorPool::CrossLinkOnDemandHelper(StringPiece name,
        bool expecting_enum) const {
        (void)expecting_enum; // Parameter is used by Google-internal code.
        auto lookup_name = std::string(name);
        if (!lookup_name.empty() && lookup_name[0] == '.') {
            lookup_name = lookup_name.substr(1);
        }
        Symbol result = tables_->FindByNameHelper(this, lookup_name);
        return result;
    }


    static bool ValidateQualifiedName(StringPiece name) {
        bool last_was_period = false;

        for (char character : name) {
            // I don't trust isalnum() due to locales.  :(
            if (('a' <= character && character <= 'z') || ('A' <= character && character <= 'Z') || ('0' <= character && character <= '9') || (character == '_')) {
                last_was_period = false;
            } else if (character == '.') {
                if (last_was_period)
                    return false;
                last_was_period = true;
            } else {
                return false;
            }
        }

        return !name.empty() && !last_was_period;
    }

    Symbol DescriptorPool::NewPlaceholder(StringPiece name,
        PlaceholderType placeholder_type) const {
        MutexLockMaybe lock(mutex_);
        return NewPlaceholderWithMutexHeld(name, placeholder_type);
    }

    Symbol DescriptorPool::NewPlaceholderWithMutexHeld(
        StringPiece name, PlaceholderType placeholder_type) const {
        if (mutex_) {
            mutex_->AssertHeld();
        }
        // Compute names.
        StringPiece placeholder_full_name;
        StringPiece placeholder_name;
        const std::string* placeholder_package;

        if (!ValidateQualifiedName(name))
            return Symbol();
        if (name[0] == '.') {
            // Fully-qualified.
            placeholder_full_name = name.substr(1);
        } else {
            placeholder_full_name = name;
        }

        // Create the placeholders.
        internal::FlatAllocator alloc;
        alloc.PlanArray<FileDescriptor>(1);
        alloc.PlanArray<std::string>(2);
        if (placeholder_type == PLACEHOLDER_ENUM) {
            alloc.PlanArray<EnumDescriptor>(1);
            alloc.PlanArray<EnumValueDescriptor>(1);
            alloc.PlanArray<std::string>(2); // names for the descriptor.
            alloc.PlanArray<std::string>(2); // names for the value.
        } else {
            alloc.PlanArray<Descriptor>(1);
            alloc.PlanArray<std::string>(2); // names for the descriptor.
            if (placeholder_type == PLACEHOLDER_EXTENDABLE_MESSAGE) {
                alloc.PlanArray<Descriptor::ExtensionRange>(1);
            }
        }
        alloc.FinalizePlanning(tables_);

        const std::string::size_type dotpos = placeholder_full_name.find_last_of('.');
        if (dotpos != std::string::npos) {
            placeholder_package = alloc.AllocateStrings(placeholder_full_name.substr(0, dotpos));
            placeholder_name = placeholder_full_name.substr(dotpos + 1);
        } else {
            placeholder_package = alloc.AllocateStrings("");
            placeholder_name = placeholder_full_name;
        }

        FileDescriptor* placeholder_file = NewPlaceholderFileWithMutexHeld(
            StrCat(placeholder_full_name, ".placeholder.shark"), alloc);
        placeholder_file->package_ = placeholder_package;

        if (placeholder_type == PLACEHOLDER_ENUM) {
            placeholder_file->enum_type_count_ = 1;
            placeholder_file->enum_types_ = alloc.AllocateArray<EnumDescriptor>(1);

            EnumDescriptor* placeholder_enum = &placeholder_file->enum_types_[0];
            memset(static_cast<void*>(placeholder_enum), 0, sizeof(*placeholder_enum));

            placeholder_enum->all_names_ = alloc.AllocateStrings(placeholder_name, placeholder_full_name);
            placeholder_enum->file_ = placeholder_file;
            placeholder_enum->options_ = &EnumOptions::default_instance();
            placeholder_enum->is_placeholder_ = true;
            placeholder_enum->is_unqualified_placeholder_ = (name[0] != '.');

            // Enums must have at least one value.
            placeholder_enum->value_count_ = 1;
            placeholder_enum->values_ = alloc.AllocateArray<EnumValueDescriptor>(1);
            // Disable fast-path lookup for this enum.
            placeholder_enum->sequential_value_limit_ = -1;

            EnumValueDescriptor* placeholder_value = &placeholder_enum->values_[0];
            memset(static_cast<void*>(placeholder_value), 0,
                sizeof(*placeholder_value));

            // Note that enum value names are siblings of their type, not children.
            placeholder_value->all_names_ = alloc.AllocateStrings(
                "PLACEHOLDER_VALUE", placeholder_package->empty() ? "PLACEHOLDER_VALUE" : *placeholder_package + ".PLACEHOLDER_VALUE");

            placeholder_value->number_ = 0;
            placeholder_value->type_ = placeholder_enum;
            placeholder_value->options_ = &EnumValueOptions::default_instance();

            return Symbol(placeholder_enum);
        } else {
            placeholder_file->message_type_count_ = 1;
            placeholder_file->message_types_ = alloc.AllocateArray<Descriptor>(1);

            Descriptor* placeholder_message = &placeholder_file->message_types_[0];
            memset(static_cast<void*>(placeholder_message), 0,
                sizeof(*placeholder_message));

            placeholder_message->all_names_ = alloc.AllocateStrings(placeholder_name, placeholder_full_name);
            placeholder_message->file_ = placeholder_file;
            placeholder_message->options_ = &MessageOptions::default_instance();
            placeholder_message->is_placeholder_ = true;
            placeholder_message->is_unqualified_placeholder_ = (name[0] != '.');

            if (placeholder_type == PLACEHOLDER_EXTENDABLE_MESSAGE) {
                placeholder_message->extension_range_count_ = 1;
                placeholder_message->extension_ranges_ = alloc.AllocateArray<Descriptor::ExtensionRange>(1);
                placeholder_message->extension_ranges_[0].start = 1;
                // kMaxNumber + 1 because ExtensionRange::end is exclusive.
                placeholder_message->extension_ranges_[0].end = FieldDescriptor::kMaxNumber + 1;
                placeholder_message->extension_ranges_[0].options_ = nullptr;
            }

            return Symbol(placeholder_message);
        }
    }

    FileDescriptor* DescriptorPool::NewPlaceholderFile(
        StringPiece name) const {
        MutexLockMaybe lock(mutex_);
        internal::FlatAllocator alloc;
        alloc.PlanArray<FileDescriptor>(1);
        alloc.PlanArray<std::string>(1);
        alloc.FinalizePlanning(tables_);

        return NewPlaceholderFileWithMutexHeld(name, alloc);
    }

    FileDescriptor* DescriptorPool::NewPlaceholderFileWithMutexHeld(
        StringPiece name, internal::FlatAllocator& alloc) const {
        if (mutex_) {
            mutex_->AssertHeld();
        }
        FileDescriptor* placeholder = alloc.AllocateArray<FileDescriptor>(1);
        memset(static_cast<void*>(placeholder), 0, sizeof(*placeholder));

        placeholder->name_ = alloc.AllocateStrings(name);
        placeholder->package_ = &internal::GetEmptyString();
        placeholder->pool_ = this;
        placeholder->options_ = &FileOptions::default_instance();
        placeholder->tables_ = &FileDescriptorTables::GetEmptyInstance();
        placeholder->source_code_info_ = &SourceCodeInfo::default_instance();
        placeholder->is_placeholder_ = true;
        placeholder->syntax_ = SyntaxEdition::SYNTAX_UNKNOWN;
        placeholder->finished_building_ = true;
        // All other fields are zero or nullptr.

        return placeholder;
    }



} // namespace shark


#include <shark/port_undef.inc>
