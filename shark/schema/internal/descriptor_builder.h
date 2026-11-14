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
#include <shark/schema/internal/descriptor.h>
#include <shark/schema/internal/descriptor_pool_tables.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // Represents an options message to interpret. Extension names in the option
    // name are resolved relative to name_scope. element_name and orig_opt are
    // used only for error reporting (since the parser records locations against
    // pointers in the original options, not the mutable copy). The Message must be
    // one of the Options messages in descriptor.shark.
    struct OptionsToInterpret {
        OptionsToInterpret(const std::string& ns, const std::string& el,
            const std::vector<int>& path, const Message* orig_opt,
            Message* opt)
            : name_scope(ns)
            , element_name(el)
            , element_path(path)
            , original_options(orig_opt)
            , options(opt) { }
        std::string name_scope;
        std::string element_name;
        std::vector<int> element_path;
        const Message* original_options;
        Message* options;
    };

      class DescriptorBuilder {
    public:
        DescriptorBuilder(const DescriptorPool* pool, DescriptorPool::Tables* tables,
            DescriptorPool::ErrorCollector* error_collector);
        ~DescriptorBuilder();

        const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

    private:
        friend class OptionInterpreter;

        // Non-recursive part of BuildFile functionality.
        FileDescriptor* BuildFileImpl(const FileDescriptorProto& proto,
            internal::FlatAllocator& alloc);

        const DescriptorPool* pool_;
        DescriptorPool::Tables* tables_; // for convenience
        DescriptorPool::ErrorCollector* error_collector_;

        // As we build descriptors we store copies of the options messages in
        // them. We put pointers to those copies in this vector, as we build, so we
        // can later (after cross-linking) interpret those options.
        std::vector<OptionsToInterpret> options_to_interpret_;

        bool had_errors_;
        std::string filename_;
        FileDescriptor* file_;
        FileDescriptorTables* file_tables_;
        std::set<const FileDescriptor*> dependencies_;

        struct MessageHints {
            int fields_to_suggest = 0;
            const Message* first_reason = nullptr;
            DescriptorPool::ErrorCollector::ErrorLocation first_reason_location = DescriptorPool::ErrorCollector::ErrorLocation::OTHER;

            void RequestHintOnFieldNumbers(
                const Message& reason,
                DescriptorPool::ErrorCollector::ErrorLocation reason_location,
                int range_start = 0, int range_end = 1) {
                auto fit = [](int value) {
                    return std::min(std::max(value, 0), FieldDescriptor::kMaxNumber);
                };
                fields_to_suggest = fit(fields_to_suggest + fit(fit(range_end) - fit(range_start)));
                if (first_reason)
                    return;
                first_reason = &reason;
                first_reason_location = reason_location;
            }
        };

        std::unordered_map<const Descriptor*, MessageHints> message_hints_;

        // unused_dependency_ is used to record the unused imported files.
        // Note: public import is not considered.
        std::set<const FileDescriptor*> unused_dependency_;

        // If LookupSymbol() finds a symbol that is in a file which is not a declared
        // dependency of this file, it will fail, but will set
        // possible_undeclared_dependency_ to point at that file.  This is only used
        // by AddNotDefinedError() to report a more useful error message.
        // possible_undeclared_dependency_name_ is the name of the symbol that was
        // actually found in possible_undeclared_dependency_, which may be a parent
        // of the symbol actually looked for.
        const FileDescriptor* possible_undeclared_dependency_;
        std::string possible_undeclared_dependency_name_;

        // If LookupSymbol() could resolve a symbol which is not defined,
        // record the resolved name.  This is only used by AddNotDefinedError()
        // to report a more useful error message.
        std::string undefine_resolved_name_;

        // Tracker for current recursion depth to implement recursion protection.
        //
        // Counts down to 0 when there is no depth remaining.
        //
        // Maximum recursion depth corresponds to 32 nested message declarations.
        int recursion_depth_ = 32;

        void AddError(const std::string& element_name, const Message& descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location,
            const std::string& error);
        void AddError(const std::string& element_name, const Message& descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location,
            const char* error);
        void AddRecursiveImportError(const FileDescriptorProto& proto, int from_here);
        void AddTwiceListedError(const FileDescriptorProto& proto, int index);
        void AddImportError(const FileDescriptorProto& proto, int index);

        // Adds an error indicating that undefined_symbol was not defined.  Must
        // only be called after LookupSymbol() fails.
        void AddNotDefinedError(
            const std::string& element_name, const Message& descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location,
            const std::string& undefined_symbol);

        void AddWarning(const std::string& element_name, const Message& descriptor,
            DescriptorPool::ErrorCollector::ErrorLocation location,
            const std::string& error);

        // Silly helper which determines if the given file is in the given package.
        // I.e., either file->package() == package_name or file->package() is a
        // nested package within package_name.
        bool IsInPackage(const FileDescriptor* file, const std::string& package_name);

        // Helper function which finds all public dependencies of the given file, and
        // stores the them in the dependencies_ set in the builder.
        void RecordPublicDependencies(const FileDescriptor* file);

        // Like tables_->FindSymbol(), but additionally:
        // - Search the pool's underlay if not found in tables_.
        // - Insure that the resulting Symbol is from one of the file's declared
        //   dependencies.
        Symbol FindSymbol(const std::string& name, bool build_it = true);

        // Like FindSymbol() but does not require that the symbol is in one of the
        // file's declared dependencies.
        Symbol FindSymbolNotEnforcingDeps(const std::string& name,
            bool build_it = true);

        // This implements the body of FindSymbolNotEnforcingDeps().
        Symbol FindSymbolNotEnforcingDepsHelper(const DescriptorPool* pool,
            const std::string& name,
            bool build_it = true);

        // Like FindSymbol(), but looks up the name relative to some other symbol
        // name.  This first searches siblings of relative_to, then siblings of its
        // parents, etc.  For example, LookupSymbol("foo.bar", "baz.moo.corge") makes
        // the following calls, returning the first non-null result:
        // FindSymbol("baz.moo.foo.bar"), FindSymbol("baz.foo.bar"),
        // FindSymbol("foo.bar").  If AllowUnknownDependencies() has been called
        // on the DescriptorPool, this will generate a placeholder type if
        // the name is not found (unless the name itself is malformed).  The
        // placeholder_type parameter indicates what kind of placeholder should be
        // constructed in this case.  The resolve_mode parameter determines whether
        // any symbol is returned, or only symbols that are types.  Note, however,
        // that LookupSymbol may still return a non-type symbol in LOOKUP_TYPES mode,
        // if it believes that's all it could refer to.  The caller should always
        // check that it receives the type of symbol it was expecting.
        enum ResolveMode { LOOKUP_ALL,
            LOOKUP_TYPES };
        Symbol LookupSymbol(const std::string& name, const std::string& relative_to,
            DescriptorPool::PlaceholderType placeholder_type = DescriptorPool::PLACEHOLDER_MESSAGE,
            ResolveMode resolve_mode = LOOKUP_ALL,
            bool build_it = true);

        // Like LookupSymbol() but will not return a placeholder even if
        // AllowUnknownDependencies() has been used.
        Symbol LookupSymbolNoPlaceholder(const std::string& name,
            const std::string& relative_to,
            ResolveMode resolve_mode = LOOKUP_ALL,
            bool build_it = true);

        // Calls tables_->AddSymbol() and records an error if it fails.  Returns
        // true if successful or false if failed, though most callers can ignore
        // the return value since an error has already been recorded.
        bool AddSymbol(const std::string& full_name, const void* parent,
            const std::string& name, const Message& proto, Symbol symbol);

        // Like AddSymbol(), but succeeds if the symbol is already defined as long
        // as the existing definition is also a package (because it's OK to define
        // the same package in two different files).  Also adds all parents of the
        // package to the symbol table (e.g. AddPackage("foo.bar", ...) will add
        // "foo.bar" and "foo" to the table).
        void AddPackage(const std::string& name, const Message& proto,
            FileDescriptor* file);

        // Checks that the symbol name contains only alphanumeric characters and
        // underscores.  Records an error otherwise.
        void ValidateSymbolName(const std::string& name, const std::string& full_name,
            const Message& proto);

        // Allocates a copy of orig_options in tables_ and stores it in the
        // descriptor. Remembers its uninterpreted options, to be interpreted
        // later. DescriptorT must be one of the Descriptor messages from
        // descriptor.shark.
        template <class DescriptorT>
        void AllocateOptions(const typename DescriptorT::OptionsType& orig_options,
            DescriptorT* descriptor, int options_field_tag,
            const std::string& option_name,
            internal::FlatAllocator& alloc);
        // Specialization for FileOptions.
        void AllocateOptions(const FileOptions& orig_options,
            FileDescriptor* descriptor,
            internal::FlatAllocator& alloc);

        // Implementation for AllocateOptions(). Don't call this directly.
        template <class DescriptorT>
        void AllocateOptionsImpl(
            const std::string& name_scope, const std::string& element_name,
            const typename DescriptorT::OptionsType& orig_options,
            DescriptorT* descriptor, const std::vector<int>& options_path,
            const std::string& option_name, internal::FlatAllocator& alloc);

        // Allocates an array of two strings, the first one is a copy of `proto_name`,
        // and the second one is the full name.
        // Full proto name is "scope.proto_name" if scope is non-empty and
        // "proto_name" otherwise.
        const std::string* AllocateNameStrings(const std::string& scope,
            const std::string& proto_name,
            internal::FlatAllocator& alloc);

        // These methods all have the same signature for the sake of the BUILD_ARRAY
        // macro, below.
        void BuildMessage(const DescriptorProto& proto, const Descriptor* parent,
            Descriptor* result, internal::FlatAllocator& alloc);
        void BuildFieldOrExtension(const FieldDescriptorProto& proto,
            Descriptor* parent, FieldDescriptor* result,
            bool is_extension, internal::FlatAllocator& alloc);
        void BuildField(const FieldDescriptorProto& proto, Descriptor* parent,
            FieldDescriptor* result, internal::FlatAllocator& alloc) {
            BuildFieldOrExtension(proto, parent, result, false, alloc);
        }
        void BuildExtension(const FieldDescriptorProto& proto, Descriptor* parent,
            FieldDescriptor* result, internal::FlatAllocator& alloc) {
            BuildFieldOrExtension(proto, parent, result, true, alloc);
        }
        void BuildExtensionRange(const DescriptorProto::ExtensionRange& proto,
            const Descriptor* parent,
            Descriptor::ExtensionRange* result,
            internal::FlatAllocator& alloc);
        void BuildReservedRange(const DescriptorProto::ReservedRange& proto,
            const Descriptor* parent,
            Descriptor::ReservedRange* result,
            internal::FlatAllocator& alloc);
        void BuildReservedRange(const EnumDescriptorProto::EnumReservedRange& proto,
            const EnumDescriptor* parent,
            EnumDescriptor::ReservedRange* result,
            internal::FlatAllocator& alloc);
        void BuildOneof(const OneofDescriptorProto& proto, Descriptor* parent,
            OneofDescriptor* result, internal::FlatAllocator& alloc);
        void CheckEnumValueUniqueness(const EnumDescriptorProto& proto,
            const EnumDescriptor* result);
        void BuildEnum(const EnumDescriptorProto& proto, const Descriptor* parent,
            EnumDescriptor* result, internal::FlatAllocator& alloc);
        void BuildEnumValue(const EnumValueDescriptorProto& proto,
            const EnumDescriptor* parent, EnumValueDescriptor* result,
            internal::FlatAllocator& alloc);
        void BuildService(const ServiceDescriptorProto& proto, const void* dummy,
            ServiceDescriptor* result, internal::FlatAllocator& alloc);
        void BuildMethod(const MethodDescriptorProto& proto,
            const ServiceDescriptor* parent, MethodDescriptor* result,
            internal::FlatAllocator& alloc);

        void LogUnusedDependency(const FileDescriptorProto& proto,
            const FileDescriptor* result);

        // Must be run only after building.
        //
        // NOTE: Options will not be available during cross-linking, as they
        // have not yet been interpreted. Defer any handling of options to the
        // Validate*Options methods.
        void CrossLinkFile(FileDescriptor* file, const FileDescriptorProto& proto);
        void CrossLinkMessage(Descriptor* message, const DescriptorProto& proto);
        void CrossLinkField(FieldDescriptor* field,
            const FieldDescriptorProto& proto);
        void CrossLinkExtensionRange(Descriptor::ExtensionRange* range,
            const DescriptorProto::ExtensionRange& proto);
        void CrossLinkEnum(EnumDescriptor* enum_type,
            const EnumDescriptorProto& proto);
        void CrossLinkEnumValue(EnumValueDescriptor* enum_value,
            const EnumValueDescriptorProto& proto);
        void CrossLinkService(ServiceDescriptor* service,
            const ServiceDescriptorProto& proto);
        void CrossLinkMethod(MethodDescriptor* method,
            const MethodDescriptorProto& proto);
        void SuggestFieldNumbers(FileDescriptor* file,
            const FileDescriptorProto& proto);

        // Must be run only after cross-linking.
        void InterpretOptions();

        // A helper class for interpreting options.
        class OptionInterpreter {
        public:
            // Creates an interpreter that operates in the context of the pool of the
            // specified builder, which must not be nullptr. We don't take ownership of
            // the builder.
            explicit OptionInterpreter(DescriptorBuilder* builder);

            ~OptionInterpreter();

            // Interprets the uninterpreted options in the specified Options message.
            // On error, calls AddError() on the underlying builder and returns false.
            // Otherwise returns true.
            bool InterpretOptions(OptionsToInterpret* options_to_interpret);

            // Updates the given source code info by re-writing uninterpreted option
            // locations to refer to the corresponding interpreted option.
            void UpdateSourceCodeInfo(SourceCodeInfo* info);

            class AggregateOptionFinder;

        private:
            // Interprets uninterpreted_option_ on the specified message, which
            // must be the mutable copy of the original options message to which
            // uninterpreted_option_ belongs. The given src_path is the source
            // location path to the uninterpreted option, and options_path is the
            // source location path to the options message. The location paths are
            // recorded and then used in UpdateSourceCodeInfo.
            bool InterpretSingleOption(Message* options,
                const std::vector<int>& src_path,
                const std::vector<int>& options_path);

            // Adds the uninterpreted_option to the given options message verbatim.
            // Used when AllowUnknownDependencies() is in effect and we can't find
            // the option's definition.
            void AddWithoutInterpreting(const UninterpretedOption& uninterpreted_option,
                Message* options);

            // A recursive helper function that drills into the intermediate fields
            // in unknown_fields to check if field innermost_field is set on the
            // innermost message. Returns false and sets an error if so.
            bool ExamineIfOptionIsSet(
                std::vector<const FieldDescriptor*>::const_iterator
                    intermediate_fields_iter,
                std::vector<const FieldDescriptor*>::const_iterator
                    intermediate_fields_end,
                const FieldDescriptor* innermost_field,
                const std::string& debug_msg_name,
                const UnknownFieldSet& unknown_fields);

            // Validates the value for the option field of the currently interpreted
            // option and then sets it on the unknown_field.
            bool SetOptionValue(const FieldDescriptor* option_field,
                UnknownFieldSet* unknown_fields);

            // Parses an aggregate value for a SharkFieldCppType::CPPTYPE_MESSAGE option and
            // saves it into *unknown_fields.
            bool SetAggregateOption(const FieldDescriptor* option_field,
                UnknownFieldSet* unknown_fields);

            // Convenience functions to set an int field the right way, depending on
            // its wire type (a single int CppType can represent multiple wire types).
            void SetInt32(int number, int32_t value, SharkFieldType type,
                UnknownFieldSet* unknown_fields);
            void SetInt64(int number, int64_t value, SharkFieldType type,
                UnknownFieldSet* unknown_fields);
            void SetUInt32(int number, uint32_t value, SharkFieldType type,
                UnknownFieldSet* unknown_fields);
            void SetUInt64(int number, uint64_t value, SharkFieldType type,
                UnknownFieldSet* unknown_fields);

            // A helper function that adds an error at the specified location of the
            // option we're currently interpreting, and returns false.
            bool AddOptionError(DescriptorPool::ErrorCollector::ErrorLocation location,
                const std::string& msg) {
                builder_->AddError(options_to_interpret_->element_name,
                    *uninterpreted_option_, location, msg);
                return false;
            }

            // A helper function that adds an error at the location of the option name
            // and returns false.
            bool AddNameError(const std::string& msg) {
#ifdef SHARK_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
                return true;
#else // SHARK_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
                return AddOptionError(DescriptorPool::ErrorCollector::OPTION_NAME, msg);
#endif // SHARK_INTERNAL_IGNORE_FIELD_NAME_ERRORS_
            }

            // A helper function that adds an error at the location of the option name
            // and returns false.
            bool AddValueError(const std::string& msg) {
                return AddOptionError(DescriptorPool::ErrorCollector::OPTION_VALUE, msg);
            }

            // We interpret against this builder's pool. Is never nullptr. We don't own
            // this pointer.
            DescriptorBuilder* builder_;

            // The options we're currently interpreting, or nullptr if we're not in a
            // call to InterpretOptions.
            const OptionsToInterpret* options_to_interpret_;

            // The option we're currently interpreting within options_to_interpret_, or
            // nullptr if we're not in a call to InterpretOptions(). This points to a
            // submessage of the original option, not the mutable copy. Therefore we
            // can use it to find locations recorded by the parser.
            const UninterpretedOption* uninterpreted_option_;

            // This maps the element path of uninterpreted options to the element path
            // of the resulting interpreted option. This is used to modify a file's
            // source code info to account for option interpretation.
            std::map<std::vector<int>, std::vector<int>> interpreted_paths_;

            // This maps the path to a repeated option field to the known number of
            // elements the field contains. This is used to track the compute the
            // index portion of the element path when interpreting a single option.
            std::map<std::vector<int>, int> repeated_option_counts_;

            // Factory used to create the dynamic messages we need to parse
            // any aggregate option values we encounter.
            DynamicMessageFactory dynamic_factory_;

            SHARK_DISALLOW_EVIL_CONSTRUCTORS(OptionInterpreter);
        };

        // Work-around for broken compilers:  According to the C++ standard,
        // OptionInterpreter should have access to the private members of any class
        // which has declared DescriptorBuilder as a friend.  Unfortunately some old
        // versions of GCC and other compilers do not implement this correctly.  So,
        // we have to have these intermediate methods to provide access.  We also
        // redundantly declare OptionInterpreter a friend just to make things extra
        // clear for these bad compilers.
        friend class OptionInterpreter;
        friend class OptionInterpreter::AggregateOptionFinder;

        static inline bool get_allow_unknown(const DescriptorPool* pool) {
            return pool->allow_unknown_;
        }
        static inline bool get_enforce_weak(const DescriptorPool* pool) {
            return pool->enforce_weak_;
        }
        static inline bool get_is_placeholder(const Descriptor* descriptor) {
            return descriptor != nullptr && descriptor->is_placeholder_;
        }
        static inline void assert_mutex_held(const DescriptorPool* pool) {
            if (pool->mutex_ != nullptr) {
                pool->mutex_->AssertHeld();
            }
        }

        // Must be run only after options have been interpreted.
        //
        // NOTE: Validation code must only reference the options in the mutable
        // descriptors, which are the ones that have been interpreted. The const
        // proto references are passed in only so they can be provided to calls to
        // AddError(). Do not look at their options, which have not been interpreted.
        void ValidateFileOptions(FileDescriptor* file,
            const FileDescriptorProto& proto);
        void ValidateMessageOptions(Descriptor* message,
            const DescriptorProto& proto);
        void ValidateFieldOptions(FieldDescriptor* field,
            const FieldDescriptorProto& proto);
        void ValidateEnumOptions(EnumDescriptor* enm,
            const EnumDescriptorProto& proto);
        void ValidateEnumValueOptions(EnumValueDescriptor* enum_value,
            const EnumValueDescriptorProto& proto);
        void ValidateExtensionRangeOptions(
            const std::string& full_name, Descriptor::ExtensionRange* extension_range,
            const DescriptorProto_ExtensionRange& proto);
        void ValidateServiceOptions(ServiceDescriptor* service,
            const ServiceDescriptorProto& proto);
        void ValidateMethodOptions(MethodDescriptor* method,
            const MethodDescriptorProto& proto);
        void ValidateProto3(FileDescriptor* file, const FileDescriptorProto& proto);
        void ValidateProto3Message(Descriptor* message, const DescriptorProto& proto);
        void ValidateProto3Field(FieldDescriptor* field,
            const FieldDescriptorProto& proto);
        void ValidateProto3Enum(EnumDescriptor* enm,
            const EnumDescriptorProto& proto);

        // Returns true if the map entry message is compatible with the
        // auto-generated entry message from map fields syntax.
        bool ValidateMapEntry(FieldDescriptor* field,
            const FieldDescriptorProto& proto);

        // Recursively detects naming conflicts with map entry types for a
        // better error message.
        void DetectMapConflicts(const Descriptor* message,
            const DescriptorProto& proto);

        void ValidateJSType(FieldDescriptor* field,
            const FieldDescriptorProto& proto);
    };

       template <class DescriptorT>
    void DescriptorBuilder::AllocateOptionsImpl(
        const std::string& name_scope, const std::string& element_name,
        const typename DescriptorT::OptionsType& orig_options,
        DescriptorT* descriptor, const std::vector<int>& options_path,
        const std::string& option_name, internal::FlatAllocator& alloc) {
        auto* options = alloc.AllocateArray<typename DescriptorT::OptionsType>(1);

        if (!orig_options.IsInitialized()) {
            AddError(name_scope + "." + element_name, orig_options,
                DescriptorPool::ErrorCollector::OPTION_NAME,
                "Uninterpreted option is missing name or value.");
            return;
        }

        // Avoid using MergeFrom()/CopyFrom() in this class to make it -fno-rtti
        // friendly. Without RTTI, MergeFrom() and CopyFrom() will fallback to the
        // reflection based method, which requires the Descriptor. However, we are in
        // the middle of building the descriptors, thus the deadlock.
        options->ParseFromString(orig_options.SerializeAsString());
        descriptor->options_ = options;

        // Don't add to options_to_interpret_ unless there were uninterpreted
        // options.  This not only avoids unnecessary work, but prevents a
        // bootstrapping problem when building descriptors for descriptor.shark.
        // descriptor.shark does not contain any uninterpreted options, but
        // attempting to interpret options anyway will cause
        // OptionsType::GetDescriptor() to be called which may then deadlock since
        // we're still trying to build it.
        if (options->uninterpreted_option_size() > 0) {
            options_to_interpret_.push_back(OptionsToInterpret(
                name_scope, element_name, options_path, &orig_options, options));
        }

        // If the custom option is in unknown fields, no need to interpret it.
        // Remove the dependency file from unused_dependency.
        const UnknownFieldSet& unknown_fields = orig_options.unknown_fields();
        if (!unknown_fields.empty()) {
            // Can not use options->GetDescriptor() which may case deadlock.
            Symbol msg_symbol = tables_->FindSymbol(option_name);
            if (msg_symbol.type() == Symbol::MESSAGE) {
                for (int i = 0; i < unknown_fields.field_count(); ++i) {
                    assert_mutex_held(pool_);
                    const FieldDescriptor* field = pool_->InternalFindExtensionByNumberNoLock(
                        msg_symbol.descriptor(), unknown_fields.field(i).number());
                    if (field) {
                        unused_dependency_.erase(field->file());
                    }
                }
            }
        }
    }


} // namespace shark


#include <shark/port_undef.inc>
