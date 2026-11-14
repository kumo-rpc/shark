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

#include <shark/schema/internal/descriptor_builder.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    static const char* const kNonLinkedWeakMessageReplacementName = "shark.Empty";

    DescriptorBuilder::DescriptorBuilder(
       const DescriptorPool* pool, DescriptorPool::Tables* tables,
       DescriptorPool::ErrorCollector* error_collector)
       : pool_(pool)
       , tables_(tables)
       , error_collector_(error_collector)
       , had_errors_(false)
       , possible_undeclared_dependency_(nullptr)
       , undefine_resolved_name_("") { }

    DescriptorBuilder::~DescriptorBuilder() { }





    void DescriptorBuilder::AddError(
        const std::string& element_name, const Message& descriptor,
        DescriptorPool::ErrorCollector::ErrorLocation location,
        const std::string& error) {
        if (error_collector_ == nullptr) {
            if (!had_errors_) {
                GOOGLE_LOG(ERROR) << "Invalid proto descriptor for file \"" << filename_
                                  << "\":";
            }
            GOOGLE_LOG(ERROR) << "  " << element_name << ": " << error;
        } else {
            error_collector_->AddError(filename_, element_name, &descriptor, location,
                error);
        }
        had_errors_ = true;
    }

    void DescriptorBuilder::AddError(
        const std::string& element_name, const Message& descriptor,
        DescriptorPool::ErrorCollector::ErrorLocation location, const char* error) {
        AddError(element_name, descriptor, location, std::string(error));
    }

    void DescriptorBuilder::AddNotDefinedError(
        const std::string& element_name, const Message& descriptor,
        DescriptorPool::ErrorCollector::ErrorLocation location,
        const std::string& undefined_symbol) {
        if (possible_undeclared_dependency_ == nullptr && undefine_resolved_name_.empty()) {
            AddError(element_name, descriptor, location,
                "\"" + undefined_symbol + "\" is not defined.");
        } else {
            if (possible_undeclared_dependency_ != nullptr) {
                AddError(element_name, descriptor, location,
                    "\"" + possible_undeclared_dependency_name_ + "\" seems to be defined in \"" + possible_undeclared_dependency_->name() + "\", which is not "
                                                                                                                                             "imported by \""
                        + filename_ + "\".  To use it here, please "
                                      "add the necessary import.");
            }
            if (!undefine_resolved_name_.empty()) {
                AddError(element_name, descriptor, location,
                    "\"" + undefined_symbol + "\" is resolved to \"" + undefine_resolved_name_ + "\", which is not defined. "
                                                                                                 "The innermost scope is searched first in name resolution. "
                                                                                                 "Consider using a leading '.'(i.e., \"."
                        + undefined_symbol + "\") to start from the outermost scope.");
            }
        }
    }


    void DescriptorBuilder::AddWarning(
        const std::string& element_name, const Message& descriptor,
        DescriptorPool::ErrorCollector::ErrorLocation location,
        const std::string& error) {
        if (error_collector_ == nullptr) {
            GOOGLE_LOG(WARNING) << filename_ << " " << element_name << ": " << error;
        } else {
            error_collector_->AddWarning(filename_, element_name, &descriptor, location,
                error);
        }
    }

    bool DescriptorBuilder::IsInPackage(const FileDescriptor* file,
        const std::string& package_name) {
        return HasPrefixString(file->package(), package_name) && (file->package().size() == package_name.size() || file->package()[package_name.size()] == '.');
    }

     void DescriptorBuilder::RecordPublicDependencies(const FileDescriptor* file) {
        if (file == nullptr || !dependencies_.insert(file).second)
            return;
        for (int i = 0; file != nullptr && i < file->public_dependency_count(); i++) {
            RecordPublicDependencies(file->public_dependency(i));
        }
    }

    Symbol DescriptorBuilder::FindSymbolNotEnforcingDepsHelper(
        const DescriptorPool* pool, const std::string& name, bool build_it) {
        // If we are looking at an underlay, we must lock its mutex_, since we are
        // accessing the underlay's tables_ directly.
        MutexLockMaybe lock((pool == pool_) ? nullptr : pool->mutex_);

        Symbol result = pool->tables_->FindSymbol(name);
        if (result.IsNull() && pool->underlay_ != nullptr) {
            // Symbol not found; check the underlay.
            result = FindSymbolNotEnforcingDepsHelper(pool->underlay_, name);
        }

        if (result.IsNull()) {
            // With lazily_build_dependencies_, a symbol lookup at cross link time is
            // not guaranteed to be successful. In most cases, build_it will be false,
            // which intentionally prevents us from building an import until it's
            // actually needed. In some cases, like registering an extension, we want
            // to build the file containing the symbol, and build_it will be set.
            // Also, build_it will be true when !lazily_build_dependencies_, to provide
            // better error reporting of missing dependencies.
            if (build_it && pool->TryFindSymbolInFallbackDatabase(name)) {
                result = pool->tables_->FindSymbol(name);
            }
        }

        return result;
    }

    Symbol DescriptorBuilder::FindSymbolNotEnforcingDeps(const std::string& name,
        bool build_it) {
        Symbol result = FindSymbolNotEnforcingDepsHelper(pool_, name, build_it);
        // Only find symbols which were defined in this file or one of its
        // dependencies.
        const FileDescriptor* file = result.GetFile();
        if (file == file_ || dependencies_.count(file) > 0) {
            unused_dependency_.erase(file);
        }
        return result;
    }

    Symbol DescriptorBuilder::FindSymbol(const std::string& name, bool build_it) {
        Symbol result = FindSymbolNotEnforcingDeps(name, build_it);

        if (result.IsNull())
            return result;

        if (!pool_->enforce_dependencies_) {
            // Hack for CompilerUpgrader, and also used for lazily_build_dependencies_
            return result;
        }

        // Only find symbols which were defined in this file or one of its
        // dependencies.
        const FileDescriptor* file = result.GetFile();
        if (file == file_ || dependencies_.count(file) > 0) {
            return result;
        }

        if (result.IsPackage()) {
            // Arg, this is overcomplicated.  The symbol is a package name.  It could
            // be that the package was defined in multiple files.  result.GetFile()
            // returns the first file we saw that used this package.  We've determined
            // that that file is not a direct dependency of the file we are currently
            // building, but it could be that some other file which *is* a direct
            // dependency also defines the same package.  We can't really rule out this
            // symbol unless none of the dependencies define it.
            if (IsInPackage(file_, name))
                return result;
            for (std::set<const FileDescriptor*>::const_iterator it = dependencies_.begin();
                it != dependencies_.end(); ++it) {
                // Note:  A dependency may be nullptr if it was not found or had errors.
                if (*it != nullptr && IsInPackage(*it, name))
                    return result;
            }
        }

        possible_undeclared_dependency_ = file;
        possible_undeclared_dependency_name_ = name;
        return Symbol();
    }



    Symbol DescriptorBuilder::LookupSymbolNoPlaceholder(
        const std::string& name, const std::string& relative_to,
        ResolveMode resolve_mode, bool build_it) {
        possible_undeclared_dependency_ = nullptr;
        undefine_resolved_name_.clear();

        if (!name.empty() && name[0] == '.') {
            // Fully-qualified name.
            return FindSymbol(name.substr(1), build_it);
        }

        // If name is something like "Foo.Bar.baz", and symbols named "Foo" are
        // defined in multiple parent scopes, we only want to find "Bar.baz" in the
        // innermost one.  E.g., the following should produce an error:
        //   message Bar { message Baz {} }
        //   message Foo {
        //     message Bar {
        //     }
        //     optional Bar.Baz baz = 1;
        //   }
        // So, we look for just "Foo" first, then look for "Bar.baz" within it if
        // found.
        std::string::size_type name_dot_pos = name.find_first_of('.');
        std::string first_part_of_name;
        if (name_dot_pos == std::string::npos) {
            first_part_of_name = name;
        } else {
            first_part_of_name = name.substr(0, name_dot_pos);
        }

        std::string scope_to_try(relative_to);

        while (true) {
            // Chop off the last component of the scope.
            std::string::size_type dot_pos = scope_to_try.find_last_of('.');
            if (dot_pos == std::string::npos) {
                return FindSymbol(name, build_it);
            } else {
                scope_to_try.erase(dot_pos);
            }

            // Append ".first_part_of_name" and try to find.
            std::string::size_type old_size = scope_to_try.size();
            scope_to_try.append(1, '.');
            scope_to_try.append(first_part_of_name);
            Symbol result = FindSymbol(scope_to_try, build_it);
            if (!result.IsNull()) {
                if (first_part_of_name.size() < name.size()) {
                    // name is a compound symbol, of which we only found the first part.
                    // Now try to look up the rest of it.
                    if (result.IsAggregate()) {
                        scope_to_try.append(name, first_part_of_name.size(),
                            name.size() - first_part_of_name.size());
                        result = FindSymbol(scope_to_try, build_it);
                        if (result.IsNull()) {
                            undefine_resolved_name_ = scope_to_try;
                        }
                        return result;
                    } else {
                        // We found a symbol but it's not an aggregate.  Continue the loop.
                    }
                } else {
                    if (resolve_mode == LOOKUP_TYPES && !result.IsType()) {
                        // We found a symbol but it's not a type.  Continue the loop.
                    } else {
                        return result;
                    }
                }
            }

            // Not found.  Remove the name so we can try again.
            scope_to_try.erase(old_size);
        }
    }

    Symbol DescriptorBuilder::LookupSymbol(
        const std::string& name, const std::string& relative_to,
        DescriptorPool::PlaceholderType placeholder_type, ResolveMode resolve_mode,
        bool build_it) {
        Symbol result = LookupSymbolNoPlaceholder(name, relative_to, resolve_mode, build_it);
        if (result.IsNull() && pool_->allow_unknown_) {
            // Not found, but AllowUnknownDependencies() is enabled.  Return a
            // placeholder instead.
            result = pool_->NewPlaceholderWithMutexHeld(name, placeholder_type);
        }
        return result;
    }

    bool DescriptorBuilder::AddSymbol(const std::string& full_name,
        const void* parent, const std::string& name,
        const Message& proto, Symbol symbol) {
        // If the caller passed nullptr for the parent, the symbol is at file scope.
        // Use its file as the parent instead.
        if (parent == nullptr)
            parent = file_;

        if (full_name.find('\0') != std::string::npos) {
            AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                "\"" + full_name + "\" contains null character.");
            return false;
        }
        if (tables_->AddSymbol(full_name, symbol)) {
            if (!file_tables_->AddAliasUnderParent(parent, name, symbol)) {
                // This is only possible if there was already an error adding something of
                // the same name.
                if (!had_errors_) {
                    GOOGLE_LOG(DFATAL) << "\"" << full_name
                                       << "\" not previously defined in "
                                          "symbols_by_name_, but was defined in "
                                          "symbols_by_parent_; this shouldn't be possible.";
                }
                return false;
            }
            return true;
        } else {
            const FileDescriptor* other_file = tables_->FindSymbol(full_name).GetFile();
            if (other_file == file_) {
                std::string::size_type dot_pos = full_name.find_last_of('.');
                if (dot_pos == std::string::npos) {
                    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                        "\"" + full_name + "\" is already defined.");
                } else {
                    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                        "\"" + full_name.substr(dot_pos + 1) + "\" is already defined in \"" + full_name.substr(0, dot_pos) + "\".");
                }
            } else {
                // Symbol seems to have been defined in a different file.
                AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                    "\"" + full_name + "\" is already defined in file \"" + (other_file == nullptr ? "null" : other_file->name()) + "\".");
            }
            return false;
        }
    }


    void DescriptorBuilder::AddPackage(const std::string& name,
        const Message& proto, FileDescriptor* file) {
        if (name.find('\0') != std::string::npos) {
            AddError(name, proto, DescriptorPool::ErrorCollector::NAME,
                "\"" + name + "\" contains null character.");
            return;
        }

        Symbol existing_symbol = tables_->FindSymbol(name);
        // It's OK to redefine a package.
        if (existing_symbol.IsNull()) {
            if (&name == &file->package()) {
                // It is the toplevel package name, so insert the descriptor directly.
                tables_->AddSymbol(file->package(), Symbol(file));
            } else {
                auto* package = tables_->Allocate<Symbol::Subpackage>();
                // If the name is the package name, then it is already in the arena.
                // If not, copy it there. It came from the call to AddPackage below.
                package->name_size = static_cast<int>(name.size());
                package->file = file;
                tables_->AddSymbol(name, Symbol(package));
            }
            // Also add parent package, if any.
            std::string::size_type dot_pos = name.find_last_of('.');
            if (dot_pos == std::string::npos) {
                // No parents.
                ValidateSymbolName(name, name, proto);
            } else {
                // Has parent.
                AddPackage(name.substr(0, dot_pos), proto, file);
                ValidateSymbolName(name.substr(dot_pos + 1), name, proto);
            }
        } else if (!existing_symbol.IsPackage()) {
            // Symbol seems to have been defined in a different file.
            const FileDescriptor* other_file = existing_symbol.GetFile();
            AddError(name, proto, DescriptorPool::ErrorCollector::NAME,
                "\"" + name + "\" is already defined (as something other than "
                              "a package) in file \""
                    + (other_file == nullptr ? "null" : other_file->name()) + "\".");
        }
    }

    void DescriptorBuilder::ValidateSymbolName(const std::string& name,
        const std::string& full_name,
        const Message& proto) {
        if (name.empty()) {
            AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                "Missing name.");
        } else {
            for (char character : name) {
                // I don't trust isalnum() due to locales.  :(
                if ((character < 'a' || 'z' < character) && (character < 'A' || 'Z' < character) && (character < '0' || '9' < character) && (character != '_')) {
                    AddError(full_name, proto, DescriptorPool::ErrorCollector::NAME,
                        "\"" + name + "\" is not a valid identifier.");
                    return;
                }
            }
        }
    }


      // This generic implementation is good for all descriptors except
    // FileDescriptor.
    template <class DescriptorT>
    void DescriptorBuilder::AllocateOptions(
        const typename DescriptorT::OptionsType& orig_options,
        DescriptorT* descriptor, int options_field_tag,
        const std::string& option_name, internal::FlatAllocator& alloc) {
        std::vector<int> options_path;
        descriptor->GetLocationPath(&options_path);
        options_path.push_back(options_field_tag);
        AllocateOptionsImpl(descriptor->full_name(), descriptor->full_name(),
            orig_options, descriptor, options_path, option_name,
            alloc);
    }

    // We specialize for FileDescriptor.
    void DescriptorBuilder::AllocateOptions(const FileOptions& orig_options,
        FileDescriptor* descriptor,
        internal::FlatAllocator& alloc) {
        std::vector<int> options_path;
        options_path.push_back(FileDescriptorProto::kOptionsFieldNumber);
        // We add the dummy token so that LookupSymbol does the right thing.
        AllocateOptionsImpl(descriptor->package() + ".dummy", descriptor->name(),
            orig_options, descriptor, options_path,
            "shark.FileOptions", alloc);
    }


// A common pattern:  We want to convert a repeated field in the descriptor
// to an array of values, calling some method to build each value.
#define BUILD_ARRAY(INPUT, OUTPUT, NAME, METHOD, PARENT)                 \
    OUTPUT->NAME##_count_ = INPUT.NAME##_size();                         \
    OUTPUT->NAME##s_ = alloc.AllocateArray<                              \
        typename std::remove_pointer<decltype(OUTPUT->NAME##s_)>::type>( \
        INPUT.NAME##_size());                                            \
    for (int i = 0; i < INPUT.NAME##_size(); i++) {                      \
        METHOD(INPUT.NAME(i), PARENT, OUTPUT->NAME##s_ + i, alloc);      \
    }

    void DescriptorBuilder::AddRecursiveImportError(
        const FileDescriptorProto& proto, int from_here) {
        std::string error_message("File recursively imports itself: ");
        for (size_t i = from_here; i < tables_->pending_files_.size(); i++) {
            error_message.append(tables_->pending_files_[i]);
            error_message.append(" -> ");
        }
        error_message.append(proto.name());

        if (static_cast<size_t>(from_here) < tables_->pending_files_.size() - 1) {
            AddError(tables_->pending_files_[from_here + 1], proto,
                DescriptorPool::ErrorCollector::IMPORT, error_message);
        } else {
            AddError(proto.name(), proto, DescriptorPool::ErrorCollector::IMPORT,
                error_message);
        }
    }

    void DescriptorBuilder::AddTwiceListedError(const FileDescriptorProto& proto,
        int index) {
        AddError(proto.dependency(index), proto,
            DescriptorPool::ErrorCollector::IMPORT,
            "Import \"" + proto.dependency(index) + "\" was listed twice.");
    }

    void DescriptorBuilder::AddImportError(const FileDescriptorProto& proto,
        int index) {
        std::string message;
        if (pool_->fallback_database_ == nullptr) {
            message = "Import \"" + proto.dependency(index) + "\" has not been loaded.";
        } else {
            message = "Import \"" + proto.dependency(index) + "\" was not found or had errors.";
        }
        AddError(proto.dependency(index), proto,
            DescriptorPool::ErrorCollector::IMPORT, message);
    }


    static bool ExistingFileMatchesProto(const FileDescriptor* existing_file,
        const FileDescriptorProto& proto) {
        FileDescriptorProto existing_proto;
        existing_file->CopyTo(&existing_proto);
        // TODO(liujisi): Remove it when CopyTo supports copying syntax params when
        // syntax="proto2".
        if (existing_file->syntax() == SyntaxEdition::SYNTAX_PROTO2 && proto.has_syntax()) {
            existing_proto.set_syntax(
                existing_file->SyntaxName(existing_file->syntax()));
        }

        return existing_proto.SerializeAsString() == proto.SerializeAsString();
    }



    // These PlanAllocationSize functions will gather into the FlatAllocator all the
    // necessary memory allocations that BuildXXX functions below will do on the
    // Tables object.
    // They *must* be kept in sync. If we miss some PlanArray call we won't have
    // enough memory and will GOOGLE_CHECK-fail.
    static void PlanAllocationSize(
        const RepeatedPtrField<EnumValueDescriptorProto>& values,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<EnumValueDescriptor>(values.size());
        alloc.PlanArray<std::string>(2 * values.size()); // name + full_name
        for (const auto& v : values) {
            if (v.has_options())
                alloc.PlanArray<EnumValueOptions>(1);
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<EnumDescriptorProto>& enums,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<EnumDescriptor>(enums.size());
        alloc.PlanArray<std::string>(2 * enums.size()); // name + full_name
        for (const auto& e : enums) {
            if (e.has_options())
                alloc.PlanArray<EnumOptions>(1);
            PlanAllocationSize(e.value(), alloc);
            alloc.PlanArray<EnumDescriptor::ReservedRange>(e.reserved_range_size());
            alloc.PlanArray<const std::string*>(e.reserved_name_size());
            alloc.PlanArray<std::string>(e.reserved_name_size());
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<OneofDescriptorProto>& oneofs,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<OneofDescriptor>(oneofs.size());
        alloc.PlanArray<std::string>(2 * oneofs.size()); // name + full_name
        for (const auto& oneof : oneofs) {
            if (oneof.has_options())
                alloc.PlanArray<OneofOptions>(1);
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<FieldDescriptorProto>& fields,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<FieldDescriptor>(fields.size());
        for (const auto& field : fields) {
            if (field.has_options())
                alloc.PlanArray<FieldOptions>(1);
            alloc.PlanFieldNames(field.name(),
                field.has_json_name() ? &field.json_name() : nullptr);
            if (field.has_default_value() && field.has_type() && (field.type() == SharkType::STRING || field.type() == SharkType::BYTES)) {
                // For the default string value.
                alloc.PlanArray<std::string>(1);
            }
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<DescriptorProto::ExtensionRange>& ranges,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<Descriptor::ExtensionRange>(ranges.size());
        for (const auto& r : ranges) {
            if (r.has_options())
                alloc.PlanArray<ExtensionRangeOptions>(1);
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<DescriptorProto>& messages,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<Descriptor>(messages.size());
        alloc.PlanArray<std::string>(2 * messages.size()); // name + full_name

        for (const auto& message : messages) {
            if (message.has_options())
                alloc.PlanArray<MessageOptions>(1);
            PlanAllocationSize(message.nested_type(), alloc);
            PlanAllocationSize(message.field(), alloc);
            PlanAllocationSize(message.extension(), alloc);
            PlanAllocationSize(message.extension_range(), alloc);
            alloc.PlanArray<Descriptor::ReservedRange>(message.reserved_range_size());
            alloc.PlanArray<const std::string*>(message.reserved_name_size());
            alloc.PlanArray<std::string>(message.reserved_name_size());
            PlanAllocationSize(message.enum_type(), alloc);
            PlanAllocationSize(message.oneof_decl(), alloc);
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<MethodDescriptorProto>& methods,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<MethodDescriptor>(methods.size());
        alloc.PlanArray<std::string>(2 * methods.size()); // name + full_name
        for (const auto& m : methods) {
            if (m.has_options())
                alloc.PlanArray<MethodOptions>(1);
        }
    }

    static void PlanAllocationSize(
        const RepeatedPtrField<ServiceDescriptorProto>& services,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<ServiceDescriptor>(services.size());
        alloc.PlanArray<std::string>(2 * services.size()); // name + full_name
        for (const auto& service : services) {
            if (service.has_options())
                alloc.PlanArray<ServiceOptions>(1);
            PlanAllocationSize(service.method(), alloc);
        }
    }

    static void PlanAllocationSize(const FileDescriptorProto& proto,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<FileDescriptor>(1);
        alloc.PlanArray<FileDescriptorTables>(1);
        alloc.PlanArray<std::string>(2); // name + package
        if (proto.has_options())
            alloc.PlanArray<FileOptions>(1);
        if (proto.has_source_code_info())
            alloc.PlanArray<SourceCodeInfo>(1);

        PlanAllocationSize(proto.service(), alloc);
        PlanAllocationSize(proto.message_type(), alloc);
        PlanAllocationSize(proto.enum_type(), alloc);
        PlanAllocationSize(proto.extension(), alloc);

        alloc.PlanArray<int>(proto.weak_dependency_size());
        alloc.PlanArray<int>(proto.public_dependency_size());
        alloc.PlanArray<const FileDescriptor*>(proto.dependency_size());
    }

    const FileDescriptor* DescriptorBuilder::BuildFile(
        const FileDescriptorProto& proto) {
        filename_ = proto.name();

        // Check if the file already exists and is identical to the one being built.
        // Note:  This only works if the input is canonical -- that is, it
        //   fully-qualifies all type names, has no UninterpretedOptions, etc.
        //   This is fine, because this idempotency "feature" really only exists to
        //   accommodate one hack in the proto1->proto2 migration layer.
        const FileDescriptor* existing_file = tables_->FindFile(filename_);
        if (existing_file != nullptr) {
            // File already in pool.  Compare the existing one to the input.
            if (ExistingFileMatchesProto(existing_file, proto)) {
                // They're identical.  Return the existing descriptor.
                return existing_file;
            }

            // Not a match.  The error will be detected and handled later.
        }

        // Check to see if this file is already on the pending files list.
        // TODO(kenton):  Allow recursive imports?  It may not work with some
        //   (most?) programming languages.  E.g., in C++, a forward declaration
        //   of a type is not sufficient to allow it to be used even in a
        //   generated header file due to inlining.  This could perhaps be
        //   worked around using tricks involving inserting #include statements
        //   mid-file, but that's pretty ugly, and I'm pretty sure there are
        //   some languages out there that do not allow recursive dependencies
        //   at all.
        for (size_t i = 0; i < tables_->pending_files_.size(); i++) {
            if (tables_->pending_files_[i] == proto.name()) {
                AddRecursiveImportError(proto, i);
                return nullptr;
            }
        }

        static const int kMaximumPackageLength = 511;
        if (proto.package().size() > kMaximumPackageLength) {
            AddError(proto.package(), proto, DescriptorPool::ErrorCollector::NAME,
                "Package name is too long");
            return nullptr;
        }

        // If we have a fallback_database_, and we aren't doing lazy import building,
        // attempt to load all dependencies now, before checkpointing tables_.  This
        // avoids confusion with recursive checkpoints.
        if (!pool_->lazily_build_dependencies_) {
            if (pool_->fallback_database_ != nullptr) {
                tables_->pending_files_.push_back(proto.name());
                for (int i = 0; i < proto.dependency_size(); i++) {
                    if (tables_->FindFile(proto.dependency(i)) == nullptr && (pool_->underlay_ == nullptr || pool_->underlay_->FindFileByName(proto.dependency(i)) == nullptr)) {
                        // We don't care what this returns since we'll find out below anyway.
                        pool_->TryFindFileInFallbackDatabase(proto.dependency(i));
                    }
                }
                tables_->pending_files_.pop_back();
            }
        }

        // Checkpoint the tables so that we can roll back if something goes wrong.
        tables_->AddCheckpoint();

        internal::FlatAllocator alloc;
        PlanAllocationSize(proto, alloc);
        alloc.FinalizePlanning(tables_);
        FileDescriptor* result = BuildFileImpl(proto, alloc);

        file_tables_->FinalizeTables();
        if (result) {
            tables_->ClearLastCheckpoint();
            result->finished_building_ = true;
            alloc.ExpectConsumed();
        } else {
            tables_->RollbackToLastCheckpoint();
        }

        return result;
    }

    FileDescriptor* DescriptorBuilder::BuildFileImpl(
        const FileDescriptorProto& proto, internal::FlatAllocator& alloc) {
        FileDescriptor* result = alloc.AllocateArray<FileDescriptor>(1);
        file_ = result;

        result->is_placeholder_ = false;
        result->finished_building_ = false;
        SourceCodeInfo* info = nullptr;
        if (proto.has_source_code_info()) {
            info = alloc.AllocateArray<SourceCodeInfo>(1);
            info->CopyFrom(proto.source_code_info());
            result->source_code_info_ = info;
        } else {
            result->source_code_info_ = &SourceCodeInfo::default_instance();
        }

        file_tables_ = alloc.AllocateArray<FileDescriptorTables>(1);
        file_->tables_ = file_tables_;

        if (!proto.has_name()) {
            AddError("", proto, DescriptorPool::ErrorCollector::OTHER,
                "Missing field: FileDescriptorProto.name.");
        }

        // TODO(liujisi): Report error when the syntax is empty after all the protos
        // have added the syntax statement.
        if (proto.syntax().empty() || proto.syntax() == "proto2") {
            file_->syntax_ = SyntaxEdition::SYNTAX_PROTO2;
        } else if (proto.syntax() == "proto3") {
            file_->syntax_ = SyntaxEdition::SYNTAX_UNKNOWN;
            AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
                "syntax \"proto3\" is not supported.");
        } else {
            file_->syntax_ = SyntaxEdition::SYNTAX_UNKNOWN;
            AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
                "Unrecognized syntax: " + proto.syntax());
        }

        result->name_ = alloc.AllocateStrings(proto.name());
        if (proto.has_package()) {
            result->package_ = alloc.AllocateStrings(proto.package());
        } else {
            // We cannot rely on proto.package() returning a valid string if
            // proto.has_package() is false, because we might be running at static
            // initialization time, in which case default values have not yet been
            // initialized.
            result->package_ = alloc.AllocateStrings("");
        }
        result->pool_ = pool_;

        if (result->name().find('\0') != std::string::npos) {
            AddError(result->name(), proto, DescriptorPool::ErrorCollector::NAME,
                "\"" + result->name() + "\" contains null character.");
            return nullptr;
        }

        // Add to tables.
        if (!tables_->AddFile(result)) {
            AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
                "A file with this name is already in the pool.");
            // Bail out early so that if this is actually the exact same file, we
            // don't end up reporting that every single symbol is already defined.
            return nullptr;
        }
        if (!result->package().empty()) {
            if (std::count(result->package().begin(), result->package().end(), '.') > kPackageLimit) {
                AddError(result->package(), proto, DescriptorPool::ErrorCollector::NAME,
                    "Exceeds Maximum Package Depth");
                return nullptr;
            }
            AddPackage(result->package(), proto, result);
        }

        // Make sure all dependencies are loaded.
        std::set<std::string> seen_dependencies;
        result->dependency_count_ = proto.dependency_size();
        result->dependencies_ = alloc.AllocateArray<const FileDescriptor*>(proto.dependency_size());
        result->dependencies_once_ = nullptr;
        unused_dependency_.clear();
        std::set<int> weak_deps;
        for (int i = 0; i < proto.weak_dependency_size(); ++i) {
            weak_deps.insert(proto.weak_dependency(i));
        }

        bool need_lazy_deps = false;
        for (int i = 0; i < proto.dependency_size(); i++) {
            if (!seen_dependencies.insert(proto.dependency(i)).second) {
                AddTwiceListedError(proto, i);
            }

            const FileDescriptor* dependency = tables_->FindFile(proto.dependency(i));
            if (dependency == nullptr && pool_->underlay_ != nullptr) {
                dependency = pool_->underlay_->FindFileByName(proto.dependency(i));
            }

            if (dependency == result) {
                // Recursive import.  dependency/result is not fully initialized, and it's
                // dangerous to try to do anything with it.  The recursive import error
                // will be detected and reported in DescriptorBuilder::BuildFile().
                return nullptr;
            }

            if (dependency == nullptr) {
                if (!pool_->lazily_build_dependencies_) {
                    if (pool_->allow_unknown_ || (!pool_->enforce_weak_ && weak_deps.find(i) != weak_deps.end())) {
                        internal::FlatAllocator lazy_dep_alloc;
                        lazy_dep_alloc.PlanArray<FileDescriptor>(1);
                        lazy_dep_alloc.PlanArray<std::string>(1);
                        lazy_dep_alloc.FinalizePlanning(tables_);
                        dependency = pool_->NewPlaceholderFileWithMutexHeld(
                            proto.dependency(i), lazy_dep_alloc);
                    } else {
                        AddImportError(proto, i);
                    }
                }
            } else {
                // Add to unused_dependency_ to track unused imported files.
                // Note: do not track unused imported files for public import.
                if (pool_->enforce_dependencies_ && (pool_->unused_import_track_files_.find(proto.name()) != pool_->unused_import_track_files_.end()) && (dependency->public_dependency_count() == 0)) {
                    unused_dependency_.insert(dependency);
                }
            }

            result->dependencies_[i] = dependency;
            if (pool_->lazily_build_dependencies_ && !dependency) {
                need_lazy_deps = true;
            }
        }
        if (need_lazy_deps) {
            int total_char_size = 0;
            for (int i = 0; i < proto.dependency_size(); i++) {
                if (result->dependencies_[i] == nullptr) {
                    total_char_size += static_cast<int>(proto.dependency(i).size());
                }
                ++total_char_size; // For NUL char
            }

            void* data = tables_->AllocateBytes(
                static_cast<int>(sizeof(internal::once_flag) + total_char_size));
            result->dependencies_once_ = ::new (data) internal::once_flag { };
            char* name_data = reinterpret_cast<char*>(result->dependencies_once_ + 1);

            for (int i = 0; i < proto.dependency_size(); i++) {
                if (result->dependencies_[i] == nullptr) {
                    memcpy(name_data, proto.dependency(i).c_str(),
                        proto.dependency(i).size());
                    name_data += proto.dependency(i).size();
                }
                *name_data++ = '\0';
            }
        }

        // Check public dependencies.
        int public_dependency_count = 0;
        result->public_dependencies_ = alloc.AllocateArray<int>(proto.public_dependency_size());
        for (int i = 0; i < proto.public_dependency_size(); i++) {
            // Only put valid public dependency indexes.
            int index = proto.public_dependency(i);
            if (index >= 0 && index < proto.dependency_size()) {
                result->public_dependencies_[public_dependency_count++] = index;
                // Do not track unused imported files for public import.
                // Calling dependency(i) builds that file when doing lazy imports,
                // need to avoid doing this. Unused dependency detection isn't done
                // when building lazily, anyways.
                if (!pool_->lazily_build_dependencies_) {
                    unused_dependency_.erase(result->dependency(index));
                }
            } else {
                AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
                    "Invalid public dependency index.");
            }
        }
        result->public_dependency_count_ = public_dependency_count;

        // Build dependency set
        dependencies_.clear();
        // We don't/can't do proper dependency error checking when
        // lazily_build_dependencies_, and calling dependency(i) will force
        // a dependency to be built, which we don't want.
        if (!pool_->lazily_build_dependencies_) {
            for (int i = 0; i < result->dependency_count(); i++) {
                RecordPublicDependencies(result->dependency(i));
            }
        }

        // Check weak dependencies.
        int weak_dependency_count = 0;
        result->weak_dependencies_ = alloc.AllocateArray<int>(proto.weak_dependency_size());
        for (int i = 0; i < proto.weak_dependency_size(); i++) {
            int index = proto.weak_dependency(i);
            if (index >= 0 && index < proto.dependency_size()) {
                result->weak_dependencies_[weak_dependency_count++] = index;
            } else {
                AddError(proto.name(), proto, DescriptorPool::ErrorCollector::OTHER,
                    "Invalid weak dependency index.");
            }
        }
        result->weak_dependency_count_ = weak_dependency_count;

        // Convert children.
        BUILD_ARRAY(proto, result, message_type, BuildMessage, nullptr);
        BUILD_ARRAY(proto, result, enum_type, BuildEnum, nullptr);
        BUILD_ARRAY(proto, result, service, BuildService, nullptr);
        BUILD_ARRAY(proto, result, extension, BuildExtension, nullptr);

        // Copy options.
        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result, alloc);
        }

        // Note that the following steps must occur in exactly the specified order.

        // Cross-link.
        CrossLinkFile(result, proto);

        if (!message_hints_.empty()) {
            SuggestFieldNumbers(result, proto);
        }

        // Interpret any remaining uninterpreted options gathered into
        // options_to_interpret_ during descriptor building.  Cross-linking has made
        // extension options known, so all interpretations should now succeed.
        if (!had_errors_) {
            OptionInterpreter option_interpreter(this);
            for (std::vector<OptionsToInterpret>::iterator iter = options_to_interpret_.begin();
                iter != options_to_interpret_.end(); ++iter) {
                option_interpreter.InterpretOptions(&(*iter));
            }
            options_to_interpret_.clear();
            if (info != nullptr) {
                option_interpreter.UpdateSourceCodeInfo(info);
            }
        }

        // Validate options. See comments at InternalSetLazilyBuildDependencies about
        // error checking and lazy import building.
        if (!had_errors_ && !pool_->lazily_build_dependencies_) {
            ValidateFileOptions(result, proto);
        }

        // Additional naming conflict check for map entry types. Only need to check
        // this if there are already errors.
        if (had_errors_) {
            for (int i = 0; i < proto.message_type_size(); ++i) {
                DetectMapConflicts(result->message_type(i), proto.message_type(i));
            }
        }

        // Again, see comments at InternalSetLazilyBuildDependencies about error
        // checking. Also, don't log unused dependencies if there were previous
        // errors, since the results might be inaccurate.
        if (!had_errors_ && !unused_dependency_.empty() && !pool_->lazily_build_dependencies_) {
            LogUnusedDependency(proto, result);
        }

        if (had_errors_) {
            return nullptr;
        } else {
            return result;
        }
    }

    const std::string* DescriptorBuilder::AllocateNameStrings(
        const std::string& scope, const std::string& proto_name,
        internal::FlatAllocator& alloc) {
        if (scope.empty()) {
            return alloc.AllocateStrings(proto_name, proto_name);
        } else {
            return alloc.AllocateStrings(proto_name,
                StrCat(scope, ".", proto_name));
        }
    }


     DescriptorBuilder::OptionInterpreter::~OptionInterpreter() { }

    bool DescriptorBuilder::OptionInterpreter::InterpretOptions(
        OptionsToInterpret* options_to_interpret) {
        // Note that these may be in different pools, so we can't use the same
        // descriptor and reflection objects on both.
        Message* options = options_to_interpret->options;
        const Message* original_options = options_to_interpret->original_options;

        bool failed = false;
        options_to_interpret_ = options_to_interpret;

        // Find the uninterpreted_option field in the mutable copy of the options
        // and clear them, since we're about to interpret them.
        const FieldDescriptor* uninterpreted_options_field = options->GetDescriptor()->FindFieldByName("uninterpreted_option");
        GOOGLE_CHECK(uninterpreted_options_field != nullptr)
            << "No field named \"uninterpreted_option\" in the Options proto.";
        options->GetReflection()->ClearField(options, uninterpreted_options_field);

        std::vector<int> src_path = options_to_interpret->element_path;
        src_path.push_back(uninterpreted_options_field->number());

        // Find the uninterpreted_option field in the original options.
        const FieldDescriptor* original_uninterpreted_options_field = original_options->GetDescriptor()->FindFieldByName(
            "uninterpreted_option");
        GOOGLE_CHECK(original_uninterpreted_options_field != nullptr)
            << "No field named \"uninterpreted_option\" in the Options proto.";

        const int num_uninterpreted_options = original_options->GetReflection()->FieldSize(
            *original_options, original_uninterpreted_options_field);
        for (int i = 0; i < num_uninterpreted_options; ++i) {
            src_path.push_back(i);
            uninterpreted_option_ = down_cast<const UninterpretedOption*>(
                &original_options->GetReflection()->GetRepeatedMessage(
                    *original_options, original_uninterpreted_options_field, i));
            if (!InterpretSingleOption(options, src_path,
                    options_to_interpret->element_path)) {
                // Error already added by InterpretSingleOption().
                failed = true;
                break;
            }
            src_path.pop_back();
        }
        // Reset these, so we don't have any dangling pointers.
        uninterpreted_option_ = nullptr;
        options_to_interpret_ = nullptr;

        if (!failed) {
            // InterpretSingleOption() added the interpreted options in the
            // UnknownFieldSet, in case the option isn't yet known to us.  Now we
            // serialize the options message and deserialize it back.  That way, any
            // option fields that we do happen to know about will get moved from the
            // UnknownFieldSet into the real fields, and thus be available right away.
            // If they are not known, that's OK too. They will get reparsed into the
            // UnknownFieldSet and wait there until the message is parsed by something
            // that does know about the options.

            // Keep the unparsed options around in case the reparsing fails.
            std::unique_ptr<Message> unparsed_options(options->New());
            options->GetReflection()->Swap(unparsed_options.get(), options);

            std::string buf;
            if (!unparsed_options->AppendToString(&buf) || !options->ParseFromString(buf)) {
                builder_->AddError(
                    options_to_interpret->element_name, *original_options,
                    DescriptorPool::ErrorCollector::OTHER,
                    "Some options could not be correctly parsed using the proto "
                    "descriptors compiled into this binary.\n"
                    "Unparsed options: "
                        + unparsed_options->ShortDebugString() + "\n"
                                                                 "Parsing attempt:  "
                        + options->ShortDebugString());
                // Restore the unparsed options.
                options->GetReflection()->Swap(unparsed_options.get(), options);
            }
        }

        return !failed;
    }

    bool DescriptorBuilder::OptionInterpreter::InterpretSingleOption(
        Message* options, const std::vector<int>& src_path,
        const std::vector<int>& options_path) {
        // First do some basic validation.
        if (uninterpreted_option_->name_size() == 0) {
            // This should never happen unless the parser has gone seriously awry or
            // someone has manually created the uninterpreted option badly.
            return AddNameError("Option must have a name.");
        }
        if (uninterpreted_option_->name(0).name_part() == "uninterpreted_option") {
            return AddNameError(
                "Option must not use reserved name "
                "\"uninterpreted_option\".");
        }

        const Descriptor* options_descriptor = nullptr;
        // Get the options message's descriptor from the builder's pool, so that we
        // get the version that knows about any extension options declared in the file
        // we're currently building. The descriptor should be there as long as the
        // file we're building imported descriptor.shark.

        // Note that we use DescriptorBuilder::FindSymbolNotEnforcingDeps(), not
        // DescriptorPool::FindMessageTypeByName() because we're already holding the
        // pool's mutex, and the latter method locks it again.  We don't use
        // FindSymbol() because files that use custom options only need to depend on
        // the file that defines the option, not descriptor.shark itself.
        Symbol symbol = builder_->FindSymbolNotEnforcingDeps(
            options->GetDescriptor()->full_name());
        options_descriptor = symbol.descriptor();
        if (options_descriptor == nullptr) {
            // The options message's descriptor was not in the builder's pool, so use
            // the standard version from the generated pool. We're not holding the
            // generated pool's mutex, so we can search it the straightforward way.
            options_descriptor = options->GetDescriptor();
        }
        GOOGLE_CHECK(options_descriptor);

        // We iterate over the name parts to drill into the submessages until we find
        // the leaf field for the option. As we drill down we remember the current
        // submessage's descriptor in |descriptor| and the next field in that
        // submessage in |field|. We also track the fields we're drilling down
        // through in |intermediate_fields|. As we go, we reconstruct the full option
        // name in |debug_msg_name|, for use in error messages.
        const Descriptor* descriptor = options_descriptor;
        const FieldDescriptor* field = nullptr;
        std::vector<const FieldDescriptor*> intermediate_fields;
        std::string debug_msg_name = "";

        std::vector<int> dest_path = options_path;

        for (int i = 0; i < uninterpreted_option_->name_size(); ++i) {
            builder_->undefine_resolved_name_.clear();
            const std::string& name_part = uninterpreted_option_->name(i).name_part();
            if (debug_msg_name.size() > 0) {
                debug_msg_name += ".";
            }
            if (uninterpreted_option_->name(i).is_extension()) {
                debug_msg_name += "(" + name_part + ")";
                // Search for the extension's descriptor as an extension in the builder's
                // pool. Note that we use DescriptorBuilder::LookupSymbol(), not
                // DescriptorPool::FindExtensionByName(), for two reasons: 1) It allows
                // relative lookups, and 2) because we're already holding the pool's
                // mutex, and the latter method locks it again.
                symbol = builder_->LookupSymbol(name_part, options_to_interpret_->name_scope);
                field = symbol.field_descriptor();
                // If we don't find the field then the field's descriptor was not in the
                // builder's pool, but there's no point in looking in the generated
                // pool. We require that you import the file that defines any extensions
                // you use, so they must be present in the builder's pool.
            } else {
                debug_msg_name += name_part;
                // Search for the field's descriptor as a regular field.
                field = descriptor->FindFieldByName(name_part);
            }

            if (field == nullptr) {
                if (get_allow_unknown(builder_->pool_)) {
                    // We can't find the option, but AllowUnknownDependencies() is enabled,
                    // so we will just leave it as uninterpreted.
                    AddWithoutInterpreting(*uninterpreted_option_, options);
                    return true;
                } else if (!(builder_->undefine_resolved_name_).empty()) {
                    // Option is resolved to a name which is not defined.
                    return AddNameError(
                        "Option \"" + debug_msg_name + "\" is resolved to \"(" + builder_->undefine_resolved_name_ + ")\", which is not defined. The innermost scope is searched first "
                                                                                                                     "in name resolution. Consider using a leading '.'(i.e., \"(."
                        + debug_msg_name.substr(1) + "\") to start from the outermost scope.");
                } else {
                    return AddNameError(
                        "Option \"" + debug_msg_name + "\" unknown. Ensure that your proto" + " definition file imports the proto which defines the option.");
                }
            } else if (field->containing_type() != descriptor) {
                if (get_is_placeholder(field->containing_type())) {
                    // The field is an extension of a placeholder type, so we can't
                    // reliably verify whether it is a valid extension to use here (e.g.
                    // we don't know if it is an extension of the correct *Options message,
                    // or if it has a valid field number, etc.).  Just leave it as
                    // uninterpreted instead.
                    AddWithoutInterpreting(*uninterpreted_option_, options);
                    return true;
                } else {
                    // This can only happen if, due to some insane misconfiguration of the
                    // pools, we find the options message in one pool but the field in
                    // another. This would probably imply a hefty bug somewhere.
                    return AddNameError("Option field \"" + debug_msg_name + "\" is not a field or extension of message \"" + descriptor->name() + "\".");
                }
            } else {
                // accumulate field numbers to form path to interpreted option
                dest_path.push_back(field->number());

                if (i < uninterpreted_option_->name_size() - 1) {
                    if (field->cpp_type() != SharkFieldCppType::CPPTYPE_MESSAGE) {
                        return AddNameError("Option \"" + debug_msg_name + "\" is an atomic type, not a message.");
                    } else if (field->is_repeated()) {
                        return AddNameError("Option field \"" + debug_msg_name + "\" is a repeated message. Repeated message "
                                                                                 "options must be initialized using an "
                                                                                 "aggregate value.");
                    } else {
                        // Drill down into the submessage.
                        intermediate_fields.push_back(field);
                        descriptor = field->message_type();
                    }
                }
            }
        }

        // We've found the leaf field. Now we use UnknownFieldSets to set its value
        // on the options message. We do so because the message may not yet know
        // about its extension fields, so we may not be able to set the fields
        // directly. But the UnknownFieldSets will serialize to the same wire-format
        // message, so reading that message back in once the extension fields are
        // known will populate them correctly.

        // First see if the option is already set.
        if (!field->is_repeated() && !ExamineIfOptionIsSet(intermediate_fields.begin(), intermediate_fields.end(), field, debug_msg_name, options->GetReflection()->GetUnknownFields(*options))) {
            return false; // ExamineIfOptionIsSet() already added the error.
        }

        // First set the value on the UnknownFieldSet corresponding to the
        // innermost message.
        std::unique_ptr<UnknownFieldSet> unknown_fields(new UnknownFieldSet());
        if (!SetOptionValue(field, unknown_fields.get())) {
            return false; // SetOptionValue() already added the error.
        }

        // Now wrap the UnknownFieldSet with UnknownFieldSets corresponding to all
        // the intermediate messages.
        for (std::vector<const FieldDescriptor*>::reverse_iterator iter = intermediate_fields.rbegin();
            iter != intermediate_fields.rend(); ++iter) {
            std::unique_ptr<UnknownFieldSet> parent_unknown_fields(
                new UnknownFieldSet());
            switch ((*iter)->type()) {
            case SharkFieldType::MESSAGE: {
                std::string* outstr = parent_unknown_fields->AddLengthDelimited((*iter)->number());
                GOOGLE_CHECK(unknown_fields->SerializeToString(outstr))
                    << "Unexpected failure while serializing option submessage "
                    << debug_msg_name << "\".";
                break;
            }

            default:
                GOOGLE_LOG(FATAL) << "Invalid wire type for SharkFieldCppType::CPPTYPE_MESSAGE: "
                                  << static_cast<int>((*iter)->type());
                return false;
            }
            unknown_fields.reset(parent_unknown_fields.release());
        }

        // Now merge the UnknownFieldSet corresponding to the top-level message into
        // the options message.
        options->GetReflection()->MutableUnknownFields(options)->MergeFrom(
            *unknown_fields);

        // record the element path of the interpreted option
        if (field->is_repeated()) {
            int index = repeated_option_counts_[dest_path]++;
            dest_path.push_back(index);
        }
        interpreted_paths_[src_path] = dest_path;

        return true;
    }

    void DescriptorBuilder::OptionInterpreter::UpdateSourceCodeInfo(
        SourceCodeInfo* info) {
        if (interpreted_paths_.empty()) {
            // nothing to do!
            return;
        }

        // We find locations that match keys in interpreted_paths_ and
        // 1) replace the path with the corresponding value in interpreted_paths_
        // 2) remove any subsequent sub-locations (sub-location is one whose path
        //    has the parent path as a prefix)
        //
        // To avoid quadratic behavior of removing interior rows as we go,
        // we keep a copy. But we don't actually copy anything until we've
        // found the first match (so if the source code info has no locations
        // that need to be changed, there is zero copy overhead).

        RepeatedPtrField<SourceCodeInfo_Location>* locs = info->mutable_location();
        RepeatedPtrField<SourceCodeInfo_Location> new_locs;
        bool copying = false;

        std::vector<int> pathv;
        bool matched = false;

        for (RepeatedPtrField<SourceCodeInfo_Location>::iterator loc = locs->begin();
            loc != locs->end(); loc++) {
            if (matched) {
                // see if this location is in the range to remove
                bool loc_matches = true;
                if (loc->path_size() < static_cast<int64_t>(pathv.size())) {
                    loc_matches = false;
                } else {
                    for (size_t j = 0; j < pathv.size(); j++) {
                        if (loc->path(j) != pathv[j]) {
                            loc_matches = false;
                            break;
                        }
                    }
                }

                if (loc_matches) {
                    // don't copy this row since it is a sub-location that we're removing
                    continue;
                }

                matched = false;
            }

            pathv.clear();
            for (int j = 0; j < loc->path_size(); j++) {
                pathv.push_back(loc->path(j));
            }

            std::map<std::vector<int>, std::vector<int>>::iterator entry = interpreted_paths_.find(pathv);

            if (entry == interpreted_paths_.end()) {
                // not a match
                if (copying) {
                    *new_locs.Add() = *loc;
                }
                continue;
            }

            matched = true;

            if (!copying) {
                // initialize the copy we are building
                copying = true;
                new_locs.Reserve(locs->size());
                for (RepeatedPtrField<SourceCodeInfo_Location>::iterator it = locs->begin();
                    it != loc; it++) {
                    *new_locs.Add() = *it;
                }
            }

            // add replacement and update its path
            SourceCodeInfo_Location* replacement = new_locs.Add();
            *replacement = *loc;
            replacement->clear_path();
            for (std::vector<int>::iterator rit = entry->second.begin();
                rit != entry->second.end(); rit++) {
                replacement->add_path(*rit);
            }
        }

        // if we made a changed copy, put it in place
        if (copying) {
            *locs = new_locs;
        }
    }

    void DescriptorBuilder::OptionInterpreter::AddWithoutInterpreting(
        const UninterpretedOption& uninterpreted_option, Message* options) {
        const FieldDescriptor* field = options->GetDescriptor()->FindFieldByName("uninterpreted_option");
        GOOGLE_CHECK(field != nullptr);

        options->GetReflection()
            ->AddMessage(options, field)
            ->CopyFrom(uninterpreted_option);
    }


    bool DescriptorBuilder::OptionInterpreter::ExamineIfOptionIsSet(
        std::vector<const FieldDescriptor*>::const_iterator
            intermediate_fields_iter,
        std::vector<const FieldDescriptor*>::const_iterator intermediate_fields_end,
        const FieldDescriptor* innermost_field, const std::string& debug_msg_name,
        const UnknownFieldSet& unknown_fields) {
        // We do linear searches of the UnknownFieldSet and its sub-groups.  This
        // should be fine since it's unlikely that any one options structure will
        // contain more than a handful of options.

        if (intermediate_fields_iter == intermediate_fields_end) {
            // We're at the innermost submessage.
            for (int i = 0; i < unknown_fields.field_count(); i++) {
                if (unknown_fields.field(i).number() == innermost_field->number()) {
                    return AddNameError("Option \"" + debug_msg_name + "\" was already set.");
                }
            }
            return true;
        }

        for (int i = 0; i < unknown_fields.field_count(); i++) {
            if (unknown_fields.field(i).number() == (*intermediate_fields_iter)->number()) {
                const UnknownField* unknown_field = &unknown_fields.field(i);
                SharkFieldType type = (*intermediate_fields_iter)->type();
                // Recurse into the next submessage.
                switch (type) {
                case SharkFieldType::MESSAGE:
                    if (unknown_field->type() == UnknownField::TYPE_LENGTH_DELIMITED) {
                        UnknownFieldSet intermediate_unknown_fields;
                        if (intermediate_unknown_fields.ParseFromString(
                                unknown_field->length_delimited())
                            && !ExamineIfOptionIsSet(intermediate_fields_iter + 1,
                                intermediate_fields_end, innermost_field,
                                debug_msg_name,
                                intermediate_unknown_fields)) {
                            return false; // Error already added.
                        }
                    }
                    break;

                default:
                    GOOGLE_LOG(FATAL) << "Invalid wire type for SharkFieldCppType::CPPTYPE_MESSAGE: " << static_cast<int>(type);
                    return false;
                }
            }
        }
        return true;
    }

    bool DescriptorBuilder::OptionInterpreter::SetOptionValue(
        const FieldDescriptor* option_field, UnknownFieldSet* unknown_fields) {
        // We switch on the CppType to validate.
        switch (option_field->cpp_type()) {
        case SharkFieldCppType::CPPTYPE_INT32:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
                    return AddValueError("Value out of range for int32 option \"" + option_field->full_name() + "\".");
                } else {
                    SetInt32(option_field->number(),
                        uninterpreted_option_->positive_int_value(),
                        option_field->type(), unknown_fields);
                }
            } else if (uninterpreted_option_->has_negative_int_value()) {
                if (uninterpreted_option_->negative_int_value() < static_cast<int64_t>(std::numeric_limits<int32_t>::min())) {
                    return AddValueError("Value out of range for int32 option \"" + option_field->full_name() + "\".");
                } else {
                    SetInt32(option_field->number(),
                        uninterpreted_option_->negative_int_value(),
                        option_field->type(), unknown_fields);
                }
            } else {
                return AddValueError("Value must be integer for int32 option \"" + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_INT8:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > 127) {
                    return AddValueError("Value out of range for int8 option \"" + option_field->full_name() + "\".");
                }
                SetInt32(option_field->number(), static_cast<int32_t>(uninterpreted_option_->positive_int_value()), option_field->type(), unknown_fields);
            } else if (uninterpreted_option_->has_negative_int_value()) {
                if (uninterpreted_option_->negative_int_value() < -128) {
                    return AddValueError("Value out of range for int8 option \"" + option_field->full_name() + "\".");
                }
                SetInt32(option_field->number(), static_cast<int32_t>(uninterpreted_option_->negative_int_value()), option_field->type(), unknown_fields);
            } else {
                return AddValueError("Value must be integer for int8 option \"" + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_INT16:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > 32767) {
                    return AddValueError("Value out of range for int16 option \"" + option_field->full_name() + "\".");
                }
                SetInt32(option_field->number(), static_cast<int32_t>(uninterpreted_option_->positive_int_value()), option_field->type(), unknown_fields);
            } else if (uninterpreted_option_->has_negative_int_value()) {
                if (uninterpreted_option_->negative_int_value() < -32768) {
                    return AddValueError("Value out of range for int16 option \"" + option_field->full_name() + "\".");
                }
                SetInt32(option_field->number(), static_cast<int32_t>(uninterpreted_option_->negative_int_value()), option_field->type(), unknown_fields);
            } else {
                return AddValueError("Value must be integer for int16 option \"" + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_INT64:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
                    return AddValueError("Value out of range for int64 option \"" + option_field->full_name() + "\".");
                } else {
                    SetInt64(option_field->number(),
                        uninterpreted_option_->positive_int_value(),
                        option_field->type(), unknown_fields);
                }
            } else if (uninterpreted_option_->has_negative_int_value()) {
                SetInt64(option_field->number(),
                    uninterpreted_option_->negative_int_value(),
                    option_field->type(), unknown_fields);
            } else {
                return AddValueError("Value must be integer for int64 option \"" + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_UINT32:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > std::numeric_limits<uint32_t>::max()) {
                    return AddValueError("Value out of range for uint32 option \"" + option_field->name() + "\".");
                } else {
                    SetUInt32(option_field->number(),
                        uninterpreted_option_->positive_int_value(),
                        option_field->type(), unknown_fields);
                }
            } else {
                return AddValueError(
                    "Value must be non-negative integer for uint32 "
                    "option \""
                    + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_UINT8:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > 255) {
                    return AddValueError("Value out of range for uint8 option \"" + option_field->full_name() + "\".");
                }
                SetUInt32(option_field->number(),
                    static_cast<uint32_t>(uninterpreted_option_->positive_int_value()),
                    option_field->type(), unknown_fields);
            } else {
                return AddValueError("Value must be non-negative integer for uint8 option \"" + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_UINT16:
            if (uninterpreted_option_->has_positive_int_value()) {
                if (uninterpreted_option_->positive_int_value() > 65535) {
                    return AddValueError("Value out of range for uint16 option \"" + option_field->full_name() + "\".");
                }
                SetUInt32(option_field->number(),
                    static_cast<uint32_t>(uninterpreted_option_->positive_int_value()),
                    option_field->type(), unknown_fields);
            } else {
                return AddValueError("Value must be non-negative integer for uint16 option \"" + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_UINT64:
            if (uninterpreted_option_->has_positive_int_value()) {
                SetUInt64(option_field->number(),
                    uninterpreted_option_->positive_int_value(),
                    option_field->type(), unknown_fields);
            } else {
                return AddValueError(
                    "Value must be non-negative integer for uint64 "
                    "option \""
                    + option_field->full_name() + "\".");
            }
            break;

        case SharkFieldCppType::CPPTYPE_FLOAT: {
            float value;
            if (uninterpreted_option_->has_double_value()) {
                value = uninterpreted_option_->double_value();
            } else if (uninterpreted_option_->has_positive_int_value()) {
                value = uninterpreted_option_->positive_int_value();
            } else if (uninterpreted_option_->has_negative_int_value()) {
                value = uninterpreted_option_->negative_int_value();
            } else {
                return AddValueError("Value must be number for float option \"" + option_field->full_name() + "\".");
            }
            unknown_fields->AddFixed32(option_field->number(),
                internal::WireFormatLite::EncodeFloat(value));
            break;
        }

        case SharkFieldCppType::CPPTYPE_DOUBLE: {
            double value;
            if (uninterpreted_option_->has_double_value()) {
                value = uninterpreted_option_->double_value();
            } else if (uninterpreted_option_->has_positive_int_value()) {
                value = uninterpreted_option_->positive_int_value();
            } else if (uninterpreted_option_->has_negative_int_value()) {
                value = uninterpreted_option_->negative_int_value();
            } else {
                return AddValueError("Value must be number for double option \"" + option_field->full_name() + "\".");
            }
            unknown_fields->AddFixed64(option_field->number(),
                internal::WireFormatLite::EncodeDouble(value));
            break;
        }

        case SharkFieldCppType::CPPTYPE_BOOL:
            uint64_t value;
            if (!uninterpreted_option_->has_identifier_value()) {
                return AddValueError(
                    "Value must be identifier for boolean option "
                    "\""
                    + option_field->full_name() + "\".");
            }
            if (uninterpreted_option_->identifier_value() == "true") {
                value = 1;
            } else if (uninterpreted_option_->identifier_value() == "false") {
                value = 0;
            } else {
                return AddValueError(
                    "Value must be \"true\" or \"false\" for boolean "
                    "option \""
                    + option_field->full_name() + "\".");
            }
            unknown_fields->AddVarint(option_field->number(), value);
            break;

        case SharkFieldCppType::CPPTYPE_ENUM: {
            if (!uninterpreted_option_->has_identifier_value()) {
                return AddValueError(
                    "Value must be identifier for enum-valued option "
                    "\""
                    + option_field->full_name() + "\".");
            }
            const EnumDescriptor* enum_type = option_field->enum_type();
            const std::string& value_name = uninterpreted_option_->identifier_value();
            const EnumValueDescriptor* enum_value = nullptr;

            if (enum_type->file()->pool() != DescriptorPool::generated_pool()) {
                // The enum value's fully-qualified name is a child of the enum's
                // name.
                std::string fully_qualified_name = enum_type->full_name();
                fully_qualified_name += ".";
                fully_qualified_name += value_name;

                // Search for the enum value's descriptor in the builder's pool. Note
                // that we use DescriptorBuilder::FindSymbolNotEnforcingDeps(), not
                // DescriptorPool::FindEnumValueByName() because we're already holding
                // the pool's mutex, and the latter method locks it again.
                Symbol symbol = builder_->FindSymbolNotEnforcingDeps(fully_qualified_name);
                if (auto* candicate_descriptor = symbol.enum_value_descriptor()) {
                    if (candicate_descriptor->type() != enum_type) {
                        return AddValueError(
                            "Enum type \"" + enum_type->full_name() + "\" has no value named \"" + value_name + "\" for option \"" + option_field->full_name() + "\". This appears to be a value from a sibling type.");
                    } else {
                        enum_value = candicate_descriptor;
                    }
                }
            } else {
                // The enum type is in the generated pool, so we can search for the
                // value there.
                enum_value = enum_type->FindValueByName(value_name);
            }

            if (enum_value == nullptr) {
                return AddValueError("Enum type \"" + option_field->enum_type()->full_name() + "\" has no value named \"" + value_name + "\" for "
                                                                                                                                         "option \""
                    + option_field->full_name() + "\".");
            } else {
                // Sign-extension is not a problem, since we cast directly from int32_t
                // to uint64_t, without first going through uint32_t.
                unknown_fields->AddVarint(
                    option_field->number(),
                    static_cast<uint64_t>(static_cast<int64_t>(enum_value->number())));
            }
            break;
        }

        case SharkFieldCppType::CPPTYPE_STRING:
            if (!uninterpreted_option_->has_string_value()) {
                return AddValueError(
                    "Value must be quoted string for string option "
                    "\""
                    + option_field->full_name() + "\".");
            }
            // The string has already been unquoted and unescaped by the parser.
            unknown_fields->AddLengthDelimited(option_field->number(),
                uninterpreted_option_->string_value());
            break;

        case SharkFieldCppType::CPPTYPE_MESSAGE:
            if (!SetAggregateOption(option_field, unknown_fields)) {
                return false;
            }
            break;
        }

        return true;
    }

    class DescriptorBuilder::OptionInterpreter::AggregateOptionFinder
        : public TextFormat::Finder {
    public:
        DescriptorBuilder* builder_;

        const Descriptor* FindAnyType(const Message& /*message*/,
            const std::string& prefix,
            const std::string& name) const override {
            if (prefix != internal::kTypeGoogleApisComPrefix && prefix != internal::kTypeGoogleProdComPrefix) {
                return nullptr;
            }
            assert_mutex_held(builder_->pool_);
            return builder_->FindSymbol(name).descriptor();
        }

        const FieldDescriptor* FindExtension(Message* message,
            const std::string& name) const override {
            assert_mutex_held(builder_->pool_);
            const Descriptor* descriptor = message->GetDescriptor();
            Symbol result = builder_->LookupSymbolNoPlaceholder(name, descriptor->full_name());
            if (auto* field = result.field_descriptor()) {
                return field;
            } else if (result.type() == Symbol::MESSAGE && descriptor->options().message_set_wire_format()) {
                const Descriptor* foreign_type = result.descriptor();
                // The text format allows MessageSet items to be specified using
                // the type name, rather than the extension identifier. If the symbol
                // lookup returned a Message, and the enclosing Message has
                // message_set_wire_format = true, then return the message set
                // extension, if one exists.
                for (int i = 0; i < foreign_type->extension_count(); i++) {
                    const FieldDescriptor* extension = foreign_type->extension(i);
                    if (extension->containing_type() == descriptor && extension->type() == SharkFieldType::MESSAGE && extension->is_optional() && extension->message_type() == foreign_type) {
                        // Found it.
                        return extension;
                    }
                }
            }
            return nullptr;
        }
    };

     namespace {

        // Helper for BuildMessage below.
        struct IncrementWhenDestroyed {
            ~IncrementWhenDestroyed() { ++to_increment; }
            int& to_increment;
        };

    } // namespace

    void DescriptorBuilder::BuildMessage(const DescriptorProto& proto,
        const Descriptor* parent,
        Descriptor* result,
        internal::FlatAllocator& alloc) {
        const std::string& scope = (parent == nullptr) ? file_->package() : parent->full_name();
        result->all_names_ = AllocateNameStrings(scope, proto.name(), alloc);
        ValidateSymbolName(proto.name(), result->full_name(), proto);

        result->file_ = file_;
        result->containing_type_ = parent;
        result->is_placeholder_ = false;
        result->is_unqualified_placeholder_ = false;
        result->well_known_type_ = Descriptor::WELLKNOWNTYPE_UNSPECIFIED;
        result->options_ = nullptr; // Set to default_instance later if necessary.

        auto it = pool_->tables_->well_known_types_.find(result->full_name());
        if (it != pool_->tables_->well_known_types_.end()) {
            result->well_known_type_ = it->second;
        }

        // Calculate the continuous sequence of fields.
        // These can be fast-path'd during lookup and don't need to be added to the
        // tables.
        // We use uint16_t to save space for sequential_field_limit_, so stop before
        // overflowing it. Worst case, we are not taking full advantage on huge
        // messages, but it is unlikely.
        result->sequential_field_limit_ = 0;
        for (int i = 0; i < std::numeric_limits<uint16_t>::max() && i < proto.field_size() && proto.field(i).number() == i + 1;
            ++i) {
            result->sequential_field_limit_ = i + 1;
        }

        // Build oneofs first so that fields and extension ranges can refer to them.
        BUILD_ARRAY(proto, result, oneof_decl, BuildOneof, result);
        BUILD_ARRAY(proto, result, field, BuildField, result);
        BUILD_ARRAY(proto, result, enum_type, BuildEnum, result);
        BUILD_ARRAY(proto, result, extension_range, BuildExtensionRange, result);
        BUILD_ARRAY(proto, result, extension, BuildExtension, result);
        BUILD_ARRAY(proto, result, reserved_range, BuildReservedRange, result);

        // Before building submessages, check recursion limit.
        --recursion_depth_;
        IncrementWhenDestroyed revert { recursion_depth_ };
        if (recursion_depth_ <= 0) {
            AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::OTHER,
                "Reached maximum recursion limit for nested messages.");
            result->nested_types_ = nullptr;
            result->nested_type_count_ = 0;
            return;
        }
        BUILD_ARRAY(proto, result, nested_type, BuildMessage, result);

        // Copy reserved names.
        int reserved_name_count = proto.reserved_name_size();
        result->reserved_name_count_ = reserved_name_count;
        result->reserved_names_ = alloc.AllocateArray<const std::string*>(reserved_name_count);
        for (int i = 0; i < reserved_name_count; ++i) {
            result->reserved_names_[i] = alloc.AllocateStrings(proto.reserved_name(i));
        }

        // Copy options.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                DescriptorProto::kOptionsFieldNumber,
                "shark.MessageOptions", alloc);
        }

        AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));

        for (int i = 0; i < proto.reserved_range_size(); i++) {
            const DescriptorProto_ReservedRange& range1 = proto.reserved_range(i);
            for (int j = i + 1; j < proto.reserved_range_size(); j++) {
                const DescriptorProto_ReservedRange& range2 = proto.reserved_range(j);
                if (range1.end() > range2.start() && range2.end() > range1.start()) {
                    AddError(result->full_name(), proto.reserved_range(i),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("Reserved range $0 to $1 overlaps with "
                                            "already-defined range $2 to $3.",
                            range2.start(), range2.end() - 1,
                            range1.start(), range1.end() - 1));
                }
            }
        }

        HASH_SET<std::string> reserved_name_set;
        for (int i = 0; i < proto.reserved_name_size(); i++) {
            const std::string& name = proto.reserved_name(i);
            if (reserved_name_set.find(name) == reserved_name_set.end()) {
                reserved_name_set.insert(name);
            } else {
                AddError(name, proto, DescriptorPool::ErrorCollector::NAME,
                    strings::Substitute("Field name \"$0\" is reserved multiple times.",
                        name));
            }
        }

        for (int i = 0; i < result->field_count(); i++) {
            const FieldDescriptor* field = result->field(i);
            for (int j = 0; j < result->extension_range_count(); j++) {
                const Descriptor::ExtensionRange* range = result->extension_range(j);
                if (range->start <= field->number() && field->number() < range->end) {
                    message_hints_[result].RequestHintOnFieldNumbers(
                        proto.extension_range(j), DescriptorPool::ErrorCollector::NUMBER);
                    AddError(
                        field->full_name(), proto.extension_range(j),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute(
                            "Extension range $0 to $1 includes field \"$2\" ($3).",
                            range->start, range->end - 1, field->name(), field->number()));
                }
            }
            for (int j = 0; j < result->reserved_range_count(); j++) {
                const Descriptor::ReservedRange* range = result->reserved_range(j);
                if (range->start <= field->number() && field->number() < range->end) {
                    message_hints_[result].RequestHintOnFieldNumbers(
                        proto.reserved_range(j), DescriptorPool::ErrorCollector::NUMBER);
                    AddError(field->full_name(), proto.reserved_range(j),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("Field \"$0\" uses reserved number $1.",
                            field->name(), field->number()));
                }
            }
            if (reserved_name_set.find(field->name()) != reserved_name_set.end()) {
                AddError(
                    field->full_name(), proto.field(i),
                    DescriptorPool::ErrorCollector::NAME,
                    strings::Substitute("Field name \"$0\" is reserved.", field->name()));
            }
        }

        // Check that extension ranges don't overlap and don't include
        // reserved field numbers or names.
        for (int i = 0; i < result->extension_range_count(); i++) {
            const Descriptor::ExtensionRange* range1 = result->extension_range(i);
            for (int j = 0; j < result->reserved_range_count(); j++) {
                const Descriptor::ReservedRange* range2 = result->reserved_range(j);
                if (range1->end > range2->start && range2->end > range1->start) {
                    AddError(result->full_name(), proto.extension_range(i),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("Extension range $0 to $1 overlaps with "
                                            "reserved range $2 to $3.",
                            range1->start, range1->end - 1, range2->start,
                            range2->end - 1));
                }
            }
            for (int j = i + 1; j < result->extension_range_count(); j++) {
                const Descriptor::ExtensionRange* range2 = result->extension_range(j);
                if (range1->end > range2->start && range2->end > range1->start) {
                    AddError(result->full_name(), proto.extension_range(i),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("Extension range $0 to $1 overlaps with "
                                            "already-defined range $2 to $3.",
                            range2->start, range2->end - 1, range1->start,
                            range1->end - 1));
                }
            }
        }
    }


    void DescriptorBuilder::BuildFieldOrExtension(const FieldDescriptorProto& proto,
        Descriptor* parent,
        FieldDescriptor* result,
        bool is_extension,
        internal::FlatAllocator& alloc) {
        const std::string& scope = (parent == nullptr) ? file_->package() : parent->full_name();

        // We allocate all names in a single array, and dedup them.
        // We remember the indices for the potentially deduped values.
        auto all_names = alloc.AllocateFieldNames(
            proto.name(), scope,
            proto.has_json_name() ? &proto.json_name() : nullptr);
        result->all_names_ = all_names.array;
        result->lowercase_name_index_ = all_names.lowercase_index;
        result->camelcase_name_index_ = all_names.camelcase_index;
        result->json_name_index_ = all_names.json_index;

        ValidateSymbolName(proto.name(), result->full_name(), proto);

        result->file_ = file_;
        result->number_ = proto.number();
        result->is_extension_ = is_extension;
        result->is_oneof_ = false;
        result->proto3_optional_ = false;
        if (proto.proto3_optional()) {
            AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::OTHER,
                "proto3_optional is not supported.");
        }

        result->has_json_name_ = proto.has_json_name();

        // Some compilers do not allow static_cast directly between two enum types,
        // so we must cast to int first.
        result->type_ = static_cast<uint8_t>(SharkTypeTrait::to_underlying(proto.type()));
        result->label_ = static_cast<uint8_t>(
            FieldCardinalityTrait::to_underlying(proto.label()));

        if (static_cast<FieldLabel>(result->label_) == FieldLabel::REQUIRED) {
            // An extension cannot have a required field (b/13365836).
            if (result->is_extension_) {
                AddError(result->full_name(), proto,
                    // Error location `TYPE`: we would really like to indicate
                    // `LABEL`, but the `ErrorLocation` enum has no entry for this,
                    // and we don't necessarily know about all implementations of the
                    // `ErrorCollector` interface to extend them to handle the new
                    // error location type properly.
                    DescriptorPool::ErrorCollector::TYPE,
                    "The extension " + result->full_name() + " cannot be required.");
            }
        }

        // Some of these may be filled in when cross-linking.
        result->containing_type_ = nullptr;
        result->type_once_ = nullptr;
        result->default_value_enum_ = nullptr;

        result->has_default_value_ = proto.has_default_value();
        if (proto.has_default_value() && result->is_repeated()) {
            AddError(result->full_name(), proto,
                DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                "Repeated fields can't have default values.");
        }

        if (proto.has_type()) {
            if (proto.has_default_value()) {
                char* end_pos = nullptr;
                switch (result->cpp_type()) {
                case SharkFieldCppType::CPPTYPE_INT32:
                case SharkFieldCppType::CPPTYPE_INT8:
                case SharkFieldCppType::CPPTYPE_INT16:
                    result->default_value_int32_t_ = strtol(proto.default_value().c_str(), &end_pos, 0);
                    break;
                case SharkFieldCppType::CPPTYPE_UINT8:
                case SharkFieldCppType::CPPTYPE_UINT16:
                    result->default_value_uint32_t_ = strtoul(proto.default_value().c_str(), &end_pos, 0);
                    break;
                case SharkFieldCppType::CPPTYPE_INT64:
                    result->default_value_int64_t_ = strto64(proto.default_value().c_str(), &end_pos, 0);
                    break;
                case SharkFieldCppType::CPPTYPE_UINT32:
                    result->default_value_uint32_t_ = strtoul(proto.default_value().c_str(), &end_pos, 0);
                    break;
                case SharkFieldCppType::CPPTYPE_UINT64:
                    result->default_value_uint64_t_ = strtou64(proto.default_value().c_str(), &end_pos, 0);
                    break;
                case SharkFieldCppType::CPPTYPE_FLOAT:
                    if (proto.default_value() == "inf") {
                        result->default_value_float_ = std::numeric_limits<float>::infinity();
                    } else if (proto.default_value() == "-inf") {
                        result->default_value_float_ = -std::numeric_limits<float>::infinity();
                    } else if (proto.default_value() == "nan") {
                        result->default_value_float_ = std::numeric_limits<float>::quiet_NaN();
                    } else {
                        result->default_value_float_ = io::SafeDoubleToFloat(
                            io::NoLocaleStrtod(proto.default_value().c_str(), &end_pos));
                    }
                    break;
                case SharkFieldCppType::CPPTYPE_DOUBLE:
                    if (proto.default_value() == "inf") {
                        result->default_value_double_ = std::numeric_limits<double>::infinity();
                    } else if (proto.default_value() == "-inf") {
                        result->default_value_double_ = -std::numeric_limits<double>::infinity();
                    } else if (proto.default_value() == "nan") {
                        result->default_value_double_ = std::numeric_limits<double>::quiet_NaN();
                    } else {
                        result->default_value_double_ = io::NoLocaleStrtod(proto.default_value().c_str(), &end_pos);
                    }
                    break;
                case SharkFieldCppType::CPPTYPE_BOOL:
                    if (proto.default_value() == "true") {
                        result->default_value_bool_ = true;
                    } else if (proto.default_value() == "false") {
                        result->default_value_bool_ = false;
                    } else {
                        AddError(result->full_name(), proto,
                            DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                            "Boolean default must be true or false.");
                    }
                    break;
                case SharkFieldCppType::CPPTYPE_ENUM:
                    // This will be filled in when cross-linking.
                    result->default_value_enum_ = nullptr;
                    break;
                case SharkFieldCppType::CPPTYPE_STRING:
                    if (result->type() == SharkFieldType::BYTES) {
                        result->default_value_string_ = alloc.AllocateStrings(
                            UnescapeCEscapeString(proto.default_value()));
                    } else {
                        result->default_value_string_ = alloc.AllocateStrings(proto.default_value());
                    }
                    break;
                case SharkFieldCppType::CPPTYPE_MESSAGE:
                    AddError(result->full_name(), proto,
                        DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                        "Messages can't have default values.");
                    result->has_default_value_ = false;
                    result->default_generated_instance_ = nullptr;
                    break;
                }

                if (end_pos != nullptr) {
                    // end_pos is only set non-null by the parsers for numeric types,
                    // above. This checks that the default was non-empty and had no extra
                    // junk after the end of the number.
                    if (proto.default_value().empty() || *end_pos != '\0') {
                        AddError(result->full_name(), proto,
                            DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                            "Couldn't parse default value \"" + proto.default_value() + "\".");
                    }
                }
            } else {
                // No explicit default value
                switch (result->cpp_type()) {
                case SharkFieldCppType::CPPTYPE_INT32:
                case SharkFieldCppType::CPPTYPE_INT8:
                case SharkFieldCppType::CPPTYPE_INT16:
                    result->default_value_int32_t_ = 0;
                    break;
                case SharkFieldCppType::CPPTYPE_UINT8:
                case SharkFieldCppType::CPPTYPE_UINT16:
                    result->default_value_uint32_t_ = 0;
                    break;
                case SharkFieldCppType::CPPTYPE_INT64:
                    result->default_value_int64_t_ = 0;
                    break;
                case SharkFieldCppType::CPPTYPE_UINT32:
                    result->default_value_uint32_t_ = 0;
                    break;
                case SharkFieldCppType::CPPTYPE_UINT64:
                    result->default_value_uint64_t_ = 0;
                    break;
                case SharkFieldCppType::CPPTYPE_FLOAT:
                    result->default_value_float_ = 0.0f;
                    break;
                case SharkFieldCppType::CPPTYPE_DOUBLE:
                    result->default_value_double_ = 0.0;
                    break;
                case SharkFieldCppType::CPPTYPE_BOOL:
                    result->default_value_bool_ = false;
                    break;
                case SharkFieldCppType::CPPTYPE_ENUM:
                    // This will be filled in when cross-linking.
                    result->default_value_enum_ = nullptr;
                    break;
                case SharkFieldCppType::CPPTYPE_STRING:
                    result->default_value_string_ = &internal::GetEmptyString();
                    break;
                case SharkFieldCppType::CPPTYPE_MESSAGE:
                    result->default_generated_instance_ = nullptr;
                    break;
                }
            }
        }

        if (result->number() <= 0) {
            message_hints_[parent].RequestHintOnFieldNumbers(
                proto, DescriptorPool::ErrorCollector::NUMBER);
            AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                "Field numbers must be positive integers.");
        } else if (!is_extension && result->number() > FieldDescriptor::kMaxNumber) {
            // Only validate that the number is within the valid field range if it is
            // not an extension. Since extension numbers are validated with the
            // extendee's valid set of extension numbers, and those are in turn
            // validated against the max allowed number, the check is unnecessary for
            // extension fields.
            // This avoids cross-linking issues that arise when attempting to check if
            // the extendee is a message_set_wire_format message, which has a higher max
            // on extension numbers.
            message_hints_[parent].RequestHintOnFieldNumbers(
                proto, DescriptorPool::ErrorCollector::NUMBER);
            AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                strings::Substitute("Field numbers cannot be greater than $0.",
                    FieldDescriptor::kMaxNumber));
        } else if (result->number() >= FieldDescriptor::kFirstReservedNumber && result->number() <= FieldDescriptor::kLastReservedNumber) {
            message_hints_[parent].RequestHintOnFieldNumbers(
                proto, DescriptorPool::ErrorCollector::NUMBER);
            AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                strings::Substitute(
                    "Field numbers $0 through $1 are reserved for the protocol "
                    "buffer library implementation.",
                    FieldDescriptor::kFirstReservedNumber,
                    FieldDescriptor::kLastReservedNumber));
        }

        if (is_extension) {
            if (!proto.has_extendee()) {
                AddError(result->full_name(), proto,
                    DescriptorPool::ErrorCollector::EXTENDEE,
                    "FieldDescriptorProto.extendee not set for extension field.");
            }

            result->scope_.extension_scope = parent;

            if (proto.has_oneof_index()) {
                AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                    "FieldDescriptorProto.oneof_index should not be set for "
                    "extensions.");
            }
        } else {
            if (proto.has_extendee()) {
                AddError(result->full_name(), proto,
                    DescriptorPool::ErrorCollector::EXTENDEE,
                    "FieldDescriptorProto.extendee set for non-extension field.");
            }

            result->containing_type_ = parent;

            if (proto.has_oneof_index()) {
                if (proto.oneof_index() < 0 || proto.oneof_index() >= parent->oneof_decl_count()) {
                    AddError(result->full_name(), proto,
                        DescriptorPool::ErrorCollector::TYPE,
                        strings::Substitute("FieldDescriptorProto.oneof_index $0 is "
                                            "out of range for type \"$1\".",
                            proto.oneof_index(), parent->name()));
                } else {
                    result->is_oneof_ = true;
                    result->scope_.containing_oneof = parent->oneof_decl(proto.oneof_index());
                }
            }
        }

        // Copy options.
        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                FieldDescriptorProto::kOptionsFieldNumber,
                "shark.FieldOptions", alloc);
        }

        AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));
    }

    void DescriptorBuilder::BuildExtensionRange(
        const DescriptorProto::ExtensionRange& proto, const Descriptor* parent,
        Descriptor::ExtensionRange* result, internal::FlatAllocator& alloc) {
        result->start = proto.start();
        result->end = proto.end();
        if (result->start <= 0) {
            message_hints_[parent].RequestHintOnFieldNumbers(
                proto, DescriptorPool::ErrorCollector::NUMBER, result->start,
                result->end);
            AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                "Extension numbers must be positive integers.");
        }

        // Checking of the upper bound of the extension range is deferred until after
        // options interpreting. This allows messages with message_set_wire_format to
        // have extensions beyond FieldDescriptor::kMaxNumber, since the extension
        // numbers are actually used as int32s in the message_set_wire_format.

        if (result->start >= result->end) {
            AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                "Extension range end number must be greater than start number.");
        }

        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            std::vector<int> options_path;
            parent->GetLocationPath(&options_path);
            options_path.push_back(DescriptorProto::kExtensionRangeFieldNumber);
            // find index of this extension range in order to compute path
            int index;
            for (index = 0; parent->extension_ranges_ + index != result; index++) {
            }
            options_path.push_back(index);
            options_path.push_back(DescriptorProto_ExtensionRange::kOptionsFieldNumber);
            AllocateOptionsImpl(parent->full_name(), parent->full_name(),
                proto.options(), result, options_path,
                "shark.ExtensionRangeOptions", alloc);
        }
    }

    void DescriptorBuilder::BuildReservedRange(
        const DescriptorProto::ReservedRange& proto, const Descriptor* parent,
        Descriptor::ReservedRange* result, internal::FlatAllocator&) {
        result->start = proto.start();
        result->end = proto.end();
        if (result->start <= 0) {
            message_hints_[parent].RequestHintOnFieldNumbers(
                proto, DescriptorPool::ErrorCollector::NUMBER, result->start,
                result->end);
            AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                "Reserved numbers must be positive integers.");
        }
    }

    void DescriptorBuilder::BuildReservedRange(
        const EnumDescriptorProto::EnumReservedRange& proto,
        const EnumDescriptor* parent, EnumDescriptor::ReservedRange* result,
        internal::FlatAllocator&) {
        result->start = proto.start();
        result->end = proto.end();

        if (result->start > result->end) {
            AddError(parent->full_name(), proto, DescriptorPool::ErrorCollector::NUMBER,
                "Reserved range end number must be greater than start number.");
        }
    }

    void DescriptorBuilder::BuildOneof(const OneofDescriptorProto& proto,
        Descriptor* parent, OneofDescriptor* result,
        internal::FlatAllocator& alloc) {
        result->all_names_ = AllocateNameStrings(parent->full_name(), proto.name(), alloc);
        ValidateSymbolName(proto.name(), result->full_name(), proto);

        result->containing_type_ = parent;

        // We need to fill these in later.
        result->field_count_ = 0;
        result->fields_ = nullptr;
        result->options_ = nullptr;

        // Copy options.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                OneofDescriptorProto::kOptionsFieldNumber,
                "shark.OneofOptions", alloc);
        }

        AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));
    }

    void DescriptorBuilder::CheckEnumValueUniqueness(
        const EnumDescriptorProto& proto, const EnumDescriptor* result) {

        // Check that enum labels are still unique when we remove the enum prefix from
        // values that have it.
        //
        // This will fail for something like:
        //
        //   enum MyEnum {
        //     MY_ENUM_FOO = 0;
        //     FOO = 1;
        //   }
        //
        // By enforcing this reasonable constraint, we allow code generators to strip
        // the prefix and/or PascalCase it without creating conflicts.  This can lead
        // to much nicer language-specific enums like:
        //
        //   enum NameType {
        //     FirstName = 1,
        //     LastName = 2,
        //   }
        //
        // Instead of:
        //
        //   enum NameType {
        //     NAME_TYPE_FIRST_NAME = 1,
        //     NAME_TYPE_LAST_NAME = 2,
        //   }
        PrefixRemover remover(result->name());
        std::map<std::string, const EnumValueDescriptor*> values;
        for (int i = 0; i < result->value_count(); i++) {
            const EnumValueDescriptor* value = result->value(i);
            std::string stripped = EnumValueToPascalCase(remover.MaybeRemove(value->name()));
            std::pair<std::map<std::string, const EnumValueDescriptor*>::iterator, bool>
                insert_result = values.insert(std::make_pair(stripped, value));
            bool inserted = insert_result.second;

            // We don't throw the error if the two conflicting symbols are identical, or
            // if they map to the same number.  In the former case, the normal symbol
            // duplication error will fire so we don't need to (and its error message
            // will make more sense). We allow the latter case so users can create
            // aliases which add or remove the prefix (code generators that do prefix
            // stripping should de-dup the labels in this case).
            if (!inserted && insert_result.first->second->name() != value->name() && insert_result.first->second->number() != value->number()) {
                std::string error_message = "Enum name " + value->name() + " has the same name as " + values[stripped]->name() + " if you ignore case and strip out the enum name prefix (if any). "
                                                                                                                                 "This is error-prone and can lead to undefined behavior. "
                                                                                                                                 "Please avoid doing this. If you are using allow_alias, please "
                                                                                                                                 "assign the same numeric value to both enums.";
                // There are proto2 enums out there with conflicting names, so to preserve
                // compatibility we issue only a warning for proto2.
                if (result->file()->syntax() == SyntaxEdition::SYNTAX_PROTO2) {
                    AddWarning(value->full_name(), proto.value(i),
                        DescriptorPool::ErrorCollector::NAME, error_message);
                } else {
                    AddError(value->full_name(), proto.value(i),
                        DescriptorPool::ErrorCollector::NAME, error_message);
                }
            }
        }
    }


    void DescriptorBuilder::BuildEnum(const EnumDescriptorProto& proto,
        const Descriptor* parent,
        EnumDescriptor* result,
        internal::FlatAllocator& alloc) {
        const std::string& scope = (parent == nullptr) ? file_->package() : parent->full_name();

        result->all_names_ = AllocateNameStrings(scope, proto.name(), alloc);
        ValidateSymbolName(proto.name(), result->full_name(), proto);
        result->file_ = file_;
        result->containing_type_ = parent;
        result->is_placeholder_ = false;
        result->is_unqualified_placeholder_ = false;

        if (proto.value_size() == 0) {
            // We cannot allow enums with no values because this would mean there
            // would be no valid default value for fields of this type.
            AddError(result->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
                "Enums must contain at least one value.");
        }

        // Calculate the continuous sequence of the labels.
        // These can be fast-path'd during lookup and don't need to be added to the
        // tables.
        // We use uint16_t to save space for sequential_value_limit_, so stop before
        // overflowing it. Worst case, we are not taking full advantage on huge
        // enums, but it is unlikely.
        for (int i = 0;
            i < std::numeric_limits<uint16_t>::max() && i < proto.value_size() &&
            // We do the math in int64_t to avoid overflows.
            proto.value(i).number() == static_cast<int64_t>(i) + proto.value(0).number();
            ++i) {
            result->sequential_value_limit_ = i;
        }

        BUILD_ARRAY(proto, result, value, BuildEnumValue, result);
        BUILD_ARRAY(proto, result, reserved_range, BuildReservedRange, result);

        // Copy reserved names.
        int reserved_name_count = proto.reserved_name_size();
        result->reserved_name_count_ = reserved_name_count;
        result->reserved_names_ = alloc.AllocateArray<const std::string*>(reserved_name_count);
        for (int i = 0; i < reserved_name_count; ++i) {
            result->reserved_names_[i] = alloc.AllocateStrings(proto.reserved_name(i));
        }

        CheckEnumValueUniqueness(proto, result);

        // Copy options.
        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                EnumDescriptorProto::kOptionsFieldNumber,
                "shark.EnumOptions", alloc);
        }

        AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));

        for (int i = 0; i < proto.reserved_range_size(); i++) {
            const EnumDescriptorProto_EnumReservedRange& range1 = proto.reserved_range(i);
            for (int j = i + 1; j < proto.reserved_range_size(); j++) {
                const EnumDescriptorProto_EnumReservedRange& range2 = proto.reserved_range(j);
                if (range1.end() >= range2.start() && range2.end() >= range1.start()) {
                    AddError(result->full_name(), proto.reserved_range(i),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("Reserved range $0 to $1 overlaps with "
                                            "already-defined range $2 to $3.",
                            range2.start(), range2.end(), range1.start(),
                            range1.end()));
                }
            }
        }

        HASH_SET<std::string> reserved_name_set;
        for (int i = 0; i < proto.reserved_name_size(); i++) {
            const std::string& name = proto.reserved_name(i);
            if (reserved_name_set.find(name) == reserved_name_set.end()) {
                reserved_name_set.insert(name);
            } else {
                AddError(name, proto, DescriptorPool::ErrorCollector::NAME,
                    strings::Substitute("Enum value \"$0\" is reserved multiple times.",
                        name));
            }
        }

        for (int i = 0; i < result->value_count(); i++) {
            const EnumValueDescriptor* value = result->value(i);
            for (int j = 0; j < result->reserved_range_count(); j++) {
                const EnumDescriptor::ReservedRange* range = result->reserved_range(j);
                if (range->start <= value->number() && value->number() <= range->end) {
                    AddError(value->full_name(), proto.reserved_range(j),
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("Enum value \"$0\" uses reserved number $1.",
                            value->name(), value->number()));
                }
            }
            if (reserved_name_set.find(value->name()) != reserved_name_set.end()) {
                AddError(
                    value->full_name(), proto.value(i),
                    DescriptorPool::ErrorCollector::NAME,
                    strings::Substitute("Enum value \"$0\" is reserved.", value->name()));
            }
        }
    }

    void DescriptorBuilder::BuildEnumValue(const EnumValueDescriptorProto& proto,
        const EnumDescriptor* parent,
        EnumValueDescriptor* result,
        internal::FlatAllocator& alloc) {
        // Note:  full_name for enum values is a child of the enum's name.
        std::string full_name;
        full_name.reserve(parent->full_name().size() + 1 + proto.name().size());
        full_name.append(parent->full_name());
        full_name.push_back('.');
        full_name.append(proto.name());

        result->all_names_ = alloc.AllocateStrings(proto.name(), std::move(full_name));
        result->number_ = proto.number();
        result->type_ = parent;

        ValidateSymbolName(proto.name(), result->full_name(), proto);

        // Copy options.
        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                EnumValueDescriptorProto::kOptionsFieldNumber,
                "shark.EnumValueOptions", alloc);
        }

        // Enum values are scoped to their enum type (C++ enum class rules):
        // the symbol lives in the enum's own namespace, keyed by the enum's
        // full name plus the value name.
        AddSymbol(result->full_name(), parent, result->name(),proto, Symbol::EnumValue(result, 0));

        // Also register an alias under the enum's containing scope (file or
        // message) keyed by "EnumName.ValueName", so that
        // FileDescriptor::FindEnumValueByName("FooEnum.FOO_ENUM_VALUE") and
        // Descriptor::FindEnumValueByName("Baz.A") resolve without falling back
        // to the DescriptorPool (which would otherwise trigger fallback-database
        // lookups on misses). parent->containing_type() is nullptr for
        // top-level enums (AddAliasUnderParent treats nullptr as file_) and the
        // containing message for nested enums. The alias key is derived from
        // full_name by stripping the package/containing-message prefix; see
        // Symbol::parent_name_key() for ENUM_VALUE_OTHER_PARENT.
        const void* outer_parent = parent->containing_type();
        // outer_key matches what ENUM_VALUE_OTHER_PARENT's parent_name_key()
        // returns, so AddAliasUnderParent's DCHECK holds.
        std::string outer_key = parent->name();
        outer_key.push_back('.');
        outer_key.append(result->name());
        file_tables_->AddAliasUnderParent(outer_parent, outer_key,
            Symbol::EnumValue(result, 1));

        // An enum is allowed to define two numbers that refer to the same value.
        // FindValueByNumber() should return the first such value, so we simply
        // ignore AddEnumValueByNumber()'s return code.
        file_tables_->AddEnumValueByNumber(result);
    }

    void DescriptorBuilder::BuildService(const ServiceDescriptorProto& proto,
        const void* /* dummy */,
        ServiceDescriptor* result,
        internal::FlatAllocator& alloc) {
        result->all_names_ = AllocateNameStrings(file_->package(), proto.name(), alloc);
        result->file_ = file_;
        ValidateSymbolName(proto.name(), result->full_name(), proto);

        BUILD_ARRAY(proto, result, method, BuildMethod, result);

        // Copy options.
        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                ServiceDescriptorProto::kOptionsFieldNumber,
                "shark.ServiceOptions", alloc);
        }

        AddSymbol(result->full_name(), nullptr, result->name(), proto,
            Symbol(result));
    }

    void DescriptorBuilder::BuildMethod(const MethodDescriptorProto& proto,
        const ServiceDescriptor* parent,
        MethodDescriptor* result,
        internal::FlatAllocator& alloc) {
        result->service_ = parent;
        result->all_names_ = AllocateNameStrings(parent->full_name(), proto.name(), alloc);

        ValidateSymbolName(proto.name(), result->full_name(), proto);

        // These will be filled in when cross-linking.
        result->input_type_.Init();
        result->output_type_.Init();

        // Copy options.
        result->options_ = nullptr; // Set to default_instance later if necessary.
        if (proto.has_options()) {
            AllocateOptions(proto.options(), result,
                MethodDescriptorProto::kOptionsFieldNumber,
                "shark.MethodOptions", alloc);
        }

        result->client_streaming_ = proto.client_streaming();
        result->server_streaming_ = proto.server_streaming();

        AddSymbol(result->full_name(), parent, result->name(), proto, Symbol(result));
    }


#undef BUILD_ARRAY



void DescriptorBuilder::CrossLinkFile(FileDescriptor* file,
        const FileDescriptorProto& proto) {
        if (file->options_ == nullptr) {
            file->options_ = &FileOptions::default_instance();
        }

        for (int i = 0; i < file->message_type_count(); i++) {
            CrossLinkMessage(&file->message_types_[i], proto.message_type(i));
        }

        for (int i = 0; i < file->extension_count(); i++) {
            CrossLinkField(&file->extensions_[i], proto.extension(i));
        }

        for (int i = 0; i < file->enum_type_count(); i++) {
            CrossLinkEnum(&file->enum_types_[i], proto.enum_type(i));
        }

        for (int i = 0; i < file->service_count(); i++) {
            CrossLinkService(&file->services_[i], proto.service(i));
        }
    }

    void DescriptorBuilder::CrossLinkMessage(Descriptor* message,
        const DescriptorProto& proto) {
        if (message->options_ == nullptr) {
            message->options_ = &MessageOptions::default_instance();
        }

        for (int i = 0; i < message->nested_type_count(); i++) {
            CrossLinkMessage(&message->nested_types_[i], proto.nested_type(i));
        }

        for (int i = 0; i < message->enum_type_count(); i++) {
            CrossLinkEnum(&message->enum_types_[i], proto.enum_type(i));
        }

        for (int i = 0; i < message->field_count(); i++) {
            CrossLinkField(&message->fields_[i], proto.field(i));
        }

        for (int i = 0; i < message->extension_count(); i++) {
            CrossLinkField(&message->extensions_[i], proto.extension(i));
        }

        for (int i = 0; i < message->extension_range_count(); i++) {
            CrossLinkExtensionRange(&message->extension_ranges_[i],
                proto.extension_range(i));
        }

        // Set up field array for each oneof.

        // First count the number of fields per oneof.
        for (int i = 0; i < message->field_count(); i++) {
            const OneofDescriptor* oneof_decl = message->field(i)->containing_oneof();
            if (oneof_decl != nullptr) {
                // Make sure fields belonging to the same oneof are defined consecutively.
                // This enables optimizations in codegens and reflection libraries to
                // skip fields in the oneof group, as only one of the field can be set.
                // Note that field_count() returns how many fields in this oneof we have
                // seen so far. field_count() > 0 guarantees that i > 0, so field(i-1) is
                // safe.
                if (oneof_decl->field_count() > 0 && message->field(i - 1)->containing_oneof() != oneof_decl) {
                    AddError(message->full_name() + "." + message->field(i - 1)->name(),
                        proto.field(i - 1), DescriptorPool::ErrorCollector::TYPE,
                        strings::Substitute(
                            "Fields in the same oneof must be defined consecutively. "
                            "\"$0\" cannot be defined before the completion of the "
                            "\"$1\" oneof definition.",
                            message->field(i - 1)->name(), oneof_decl->name()));
                }
                // Must go through oneof_decls_ array to get a non-const version of the
                // OneofDescriptor.
                auto& out_oneof_decl = message->oneof_decls_[oneof_decl->index()];
                if (out_oneof_decl.field_count_ == 0) {
                    out_oneof_decl.fields_ = message->field(i);
                }

                if (!had_errors_) {
                    // Verify that they are contiguous.
                    // This is assumed by OneofDescriptor::field(i).
                    // But only if there are no errors.
                    GOOGLE_CHECK_EQ(out_oneof_decl.fields_ + out_oneof_decl.field_count_,
                        message->field(i));
                }
                ++out_oneof_decl.field_count_;
            }
        }

        // Then verify the sizes.
        for (int i = 0; i < message->oneof_decl_count(); i++) {
            OneofDescriptor* oneof_decl = &message->oneof_decls_[i];

            if (oneof_decl->field_count() == 0) {
                AddError(message->full_name() + "." + oneof_decl->name(),
                    proto.oneof_decl(i), DescriptorPool::ErrorCollector::NAME,
                    "Oneof must have at least one field.");
            }

            if (oneof_decl->options_ == nullptr) {
                oneof_decl->options_ = &OneofOptions::default_instance();
            }
        }

        for (int i = 0; i < message->field_count(); i++) {
            const FieldDescriptor* field = message->field(i);
            if (field->proto3_optional_) {
                if (!field->containing_oneof() || !field->containing_oneof()->is_synthetic()) {
                    AddError(message->full_name(), proto.field(i),
                        DescriptorPool::ErrorCollector::OTHER,
                        "Fields with proto3_optional set must be "
                        "a member of a one-field oneof");
                }
            }
        }

        // Synthetic oneofs must be last.
        int first_synthetic = -1;
        for (int i = 0; i < message->oneof_decl_count(); i++) {
            const OneofDescriptor* oneof = message->oneof_decl(i);
            if (oneof->is_synthetic()) {
                if (first_synthetic == -1) {
                    first_synthetic = i;
                }
            } else {
                if (first_synthetic != -1) {
                    AddError(message->full_name(), proto.oneof_decl(i),
                        DescriptorPool::ErrorCollector::OTHER,
                        "Synthetic oneofs must be after all other oneofs");
                }
            }
        }

        if (first_synthetic == -1) {
            message->real_oneof_decl_count_ = message->oneof_decl_count_;
        } else {
            message->real_oneof_decl_count_ = first_synthetic;
        }
    }

    void DescriptorBuilder::CrossLinkExtensionRange(
        Descriptor::ExtensionRange* range,
        const DescriptorProto::ExtensionRange& /*proto*/) {
        if (range->options_ == nullptr) {
            range->options_ = &ExtensionRangeOptions::default_instance();
        }
    }

    void DescriptorBuilder::CrossLinkField(FieldDescriptor* field,
        const FieldDescriptorProto& proto) {
        if (field->options_ == nullptr) {
            field->options_ = &FieldOptions::default_instance();
        }

        if (proto.has_extendee()) {
            Symbol extendee = LookupSymbol(proto.extendee(), field->full_name(),
                DescriptorPool::PLACEHOLDER_EXTENDABLE_MESSAGE);
            if (extendee.IsNull()) {
                AddNotDefinedError(field->full_name(), proto,
                    DescriptorPool::ErrorCollector::EXTENDEE,
                    proto.extendee());
                return;
            } else if (extendee.type() != Symbol::MESSAGE) {
                AddError(field->full_name(), proto,
                    DescriptorPool::ErrorCollector::EXTENDEE,
                    "\"" + proto.extendee() + "\" is not a message type.");
                return;
            }
            field->containing_type_ = extendee.descriptor();

            const Descriptor::ExtensionRange* extension_range = field->containing_type()->FindExtensionRangeContainingNumber(
                field->number());

            if (extension_range == nullptr) {
                // Set of valid extension numbers for MessageSet is different (< 2^32)
                // from other extendees (< 2^29). If unknown deps are allowed, we may not
                // have that information, and wrongly deem the extension as invalid.
                auto skip_check = get_allow_unknown(pool_) && proto.extendee() == "shark.bridge.MessageSet";
                if (!skip_check) {
                    AddError(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::NUMBER,
                        strings::Substitute("\"$0\" does not declare $1 as an "
                                            "extension number.",
                            field->containing_type()->full_name(),
                            field->number()));
                }
            }
        }

        if (field->containing_oneof() != nullptr) {
            if (field->label() != FieldLabel::OPTIONAL) {
                // Note that this error will never happen when parsing .shark files.
                // It can only happen if you manually construct a FileDescriptorProto
                // that is incorrect.
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
                    "Fields of oneofs must themselves have label OPTIONAL.");
            }
        }

        if (proto.has_type_name()) {
            // Assume we are expecting a message type unless the proto contains some
            // evidence that it expects an enum type.  This only makes a difference if
            // we end up creating a placeholder.
            bool expecting_enum = (proto.type() == SharkType::ENUM) || proto.has_default_value();

            // In case of weak fields we force building the dependency. We need to know
            // if the type exist or not. If it doesn't exist we substitute Empty which
            // should only be done if the type can't be found in the generated pool.
            // TODO(gerbens) Ideally we should query the database directly to check
            // if weak fields exist or not so that we don't need to force building
            // weak dependencies. However the name lookup rules for symbols are
            // somewhat complicated, so I defer it too another CL.
            bool is_weak = !pool_->enforce_weak_ && proto.options().weak();
            bool is_lazy = pool_->lazily_build_dependencies_ && !is_weak;

            Symbol type = LookupSymbol(proto.type_name(), field->full_name(),
                expecting_enum ? DescriptorPool::PLACEHOLDER_ENUM
                               : DescriptorPool::PLACEHOLDER_MESSAGE,
                LOOKUP_TYPES, !is_lazy);

            if (type.IsNull()) {
                if (is_lazy) {
                    // Save the symbol names for later for lookup, and allocate the once
                    // object needed for the accessors.
                    const std::string& name = proto.type_name();

                    int name_sizes = static_cast<int>(name.size() + 1 + proto.default_value().size() + 1);

                    field->type_once_ = ::new (tables_->AllocateBytes(static_cast<int>(
                        sizeof(internal::once_flag) + name_sizes))) internal::once_flag { };
                    char* names = reinterpret_cast<char*>(field->type_once_ + 1);

                    memcpy(names, name.c_str(), name.size() + 1);
                    memcpy(names + name.size() + 1, proto.default_value().c_str(),
                        proto.default_value().size() + 1);

                    // AddFieldByNumber and AddExtension are done later in this function,
                    // and can/must be done if the field type was not found. The related
                    // error checking is not necessary when in lazily_build_dependencies_
                    // mode, and can't be done without building the type's descriptor,
                    // which we don't want to do.
                    file_tables_->AddFieldByNumber(field);
                    if (field->is_extension()) {
                        tables_->AddExtension(field);
                    }
                    return;
                } else {
                    // If the type is a weak type, we change the type to a shark.Empty
                    // field.
                    if (is_weak) {
                        type = FindSymbol(kNonLinkedWeakMessageReplacementName);
                    }
                    if (type.IsNull()) {
                        AddNotDefinedError(field->full_name(), proto,
                            DescriptorPool::ErrorCollector::TYPE,
                            proto.type_name());
                        return;
                    }
                }
            }

            if (!proto.has_type()) {
                // Choose field type based on symbol.
                if (type.type() == Symbol::MESSAGE) {
                    field->type_ = static_cast<uint8_t>(SharkFieldType::MESSAGE);
                } else if (type.type() == Symbol::ENUM) {
                    field->type_ = static_cast<uint8_t>(SharkFieldType::ENUM);
                } else {
                    AddError(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::TYPE,
                        "\"" + proto.type_name() + "\" is not a type.");
                    return;
                }
            }

            if (field->cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE) {
                field->type_descriptor_.message_type = type.descriptor();
                if (field->type_descriptor_.message_type == nullptr) {
                    AddError(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::TYPE,
                        "\"" + proto.type_name() + "\" is not a message type.");
                    return;
                }

                if (field->has_default_value()) {
                    AddError(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                        "Messages can't have default values.");
                }
            } else if (field->cpp_type() == SharkFieldCppType::CPPTYPE_ENUM) {
                field->type_descriptor_.enum_type = type.enum_descriptor();
                if (field->type_descriptor_.enum_type == nullptr) {
                    AddError(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::TYPE,
                        "\"" + proto.type_name() + "\" is not an enum type.");
                    return;
                }

                if (field->enum_type()->is_placeholder_) {
                    // We can't look up default values for placeholder types.  We'll have
                    // to just drop them.
                    field->has_default_value_ = false;
                }

                if (field->has_default_value()) {
                    // Ensure that the default value is an identifier. Parser cannot always
                    // verify this because it does not have complete type information.
                    // N.B. that this check yields better error messages but is not
                    // necessary for correctness (an enum symbol must be a valid identifier
                    // anyway), only for better errors.
                    if (!io::Tokenizer::IsIdentifier(proto.default_value())) {
                        AddError(field->full_name(), proto,
                            DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                            "Default value for an enum field must be an identifier.");
                    } else {
                        // We can't just use field->enum_type()->FindValueByName() here
                        // because that locks the pool's mutex, which we have already locked
                        // at this point.
                        // Enum values are scoped to their enum type, so the fully
                        // qualified name is the enum's full name plus the value name.
                        std::string fully_qualified_name = field->enum_type()->full_name();
                        fully_qualified_name += ".";
                        fully_qualified_name += proto.default_value();
                        const EnumValueDescriptor* default_value = LookupSymbolNoPlaceholder(fully_qualified_name, "")
                                                                       .enum_value_descriptor();

                        if (default_value != nullptr && default_value->type() == field->enum_type()) {
                            field->default_value_enum_ = default_value;
                        } else {
                            AddError(field->full_name(), proto,
                                DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                                "Enum type \"" + field->enum_type()->full_name() + "\" has no value named \"" + proto.default_value() + "\".");
                        }
                    }
                } else if (field->enum_type()->value_count() > 0) {
                    // All enums must have at least one value, or we would have reported
                    // an error elsewhere.  We use the first defined value as the default
                    // if a default is not explicitly defined.
                    field->default_value_enum_ = field->enum_type()->value(0);
                }
            } else {
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                    "Field with primitive type has type_name.");
            }
        } else {
            if (field->cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE || field->cpp_type() == SharkFieldCppType::CPPTYPE_ENUM) {
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                    "Field with message or enum type missing type_name.");
            }
        }

        // Add the field to the fields-by-number table.
        // Note:  We have to do this *after* cross-linking because extensions do not
        // know their containing type until now. If we're in
        // lazily_build_dependencies_ mode, we're guaranteed there's no errors, so no
        // risk to calling containing_type() or other accessors that will build
        // dependencies.
        if (!file_tables_->AddFieldByNumber(field)) {
            const FieldDescriptor* conflicting_field = file_tables_->FindFieldByNumber(
                field->containing_type(), field->number());
            std::string containing_type_name = field->containing_type() == nullptr
                ? "unknown"
                : field->containing_type()->full_name();
            if (field->is_extension()) {
                AddError(field->full_name(), proto,
                    DescriptorPool::ErrorCollector::NUMBER,
                    strings::Substitute("Extension number $0 has already been used "
                                        "in \"$1\" by extension \"$2\".",
                        field->number(), containing_type_name,
                        conflicting_field->full_name()));
            } else {
                AddError(field->full_name(), proto,
                    DescriptorPool::ErrorCollector::NUMBER,
                    strings::Substitute("Field number $0 has already been used in "
                                        "\"$1\" by field \"$2\".",
                        field->number(), containing_type_name,
                        conflicting_field->name()));
            }
        } else {
            if (field->is_extension()) {
                if (!tables_->AddExtension(field)) {
                    const FieldDescriptor* conflicting_field = tables_->FindExtension(field->containing_type(), field->number());
                    std::string containing_type_name = field->containing_type() == nullptr
                        ? "unknown"
                        : field->containing_type()->full_name();
                    std::string error_msg = strings::Substitute(
                        "Extension number $0 has already been used in \"$1\" by extension "
                        "\"$2\" defined in $3.",
                        field->number(), containing_type_name,
                        conflicting_field->full_name(), conflicting_field->file()->name());
                    // Conflicting extension numbers should be an error. However, before
                    // turning this into an error we need to fix all existing broken
                    // protos first.
                    // TODO(xiaofeng): Change this to an error.
                    AddWarning(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::NUMBER, error_msg);
                }
            }
        }
    }

    void DescriptorBuilder::CrossLinkEnum(EnumDescriptor* enum_type,
        const EnumDescriptorProto& proto) {
        if (enum_type->options_ == nullptr) {
            enum_type->options_ = &EnumOptions::default_instance();
        }

        for (int i = 0; i < enum_type->value_count(); i++) {
            CrossLinkEnumValue(&enum_type->values_[i], proto.value(i));
        }
    }

    void DescriptorBuilder::CrossLinkEnumValue(
        EnumValueDescriptor* enum_value,
        const EnumValueDescriptorProto& /* proto */) {
        if (enum_value->options_ == nullptr) {
            enum_value->options_ = &EnumValueOptions::default_instance();
        }
    }

    void DescriptorBuilder::CrossLinkService(ServiceDescriptor* service,
        const ServiceDescriptorProto& proto) {
        if (service->options_ == nullptr) {
            service->options_ = &ServiceOptions::default_instance();
        }

        for (int i = 0; i < service->method_count(); i++) {
            CrossLinkMethod(&service->methods_[i], proto.method(i));
        }
    }



    void DescriptorBuilder::CrossLinkMethod(MethodDescriptor* method,
        const MethodDescriptorProto& proto) {
        if (method->options_ == nullptr) {
            method->options_ = &MethodOptions::default_instance();
        }

        Symbol input_type = LookupSymbol(proto.input_type(), method->full_name(),
            DescriptorPool::PLACEHOLDER_MESSAGE, LOOKUP_ALL,
            !pool_->lazily_build_dependencies_);
        if (input_type.IsNull()) {
            if (!pool_->lazily_build_dependencies_) {
                AddNotDefinedError(method->full_name(), proto,
                    DescriptorPool::ErrorCollector::INPUT_TYPE,
                    proto.input_type());
            } else {
                method->input_type_.SetLazy(proto.input_type(), file_);
            }
        } else if (input_type.type() != Symbol::MESSAGE) {
            AddError(method->full_name(), proto,
                DescriptorPool::ErrorCollector::INPUT_TYPE,
                "\"" + proto.input_type() + "\" is not a message type.");
        } else {
            method->input_type_.Set(input_type.descriptor());
        }

        Symbol output_type = LookupSymbol(proto.output_type(), method->full_name(),
            DescriptorPool::PLACEHOLDER_MESSAGE, LOOKUP_ALL,
            !pool_->lazily_build_dependencies_);
        if (output_type.IsNull()) {
            if (!pool_->lazily_build_dependencies_) {
                AddNotDefinedError(method->full_name(), proto,
                    DescriptorPool::ErrorCollector::OUTPUT_TYPE,
                    proto.output_type());
            } else {
                method->output_type_.SetLazy(proto.output_type(), file_);
            }
        } else if (output_type.type() != Symbol::MESSAGE) {
            AddError(method->full_name(), proto,
                DescriptorPool::ErrorCollector::OUTPUT_TYPE,
                "\"" + proto.output_type() + "\" is not a message type.");
        } else {
            method->output_type_.Set(output_type.descriptor());
        }
    }

    void DescriptorBuilder::SuggestFieldNumbers(FileDescriptor* file,
        const FileDescriptorProto& proto) {
        for (int message_index = 0; message_index < file->message_type_count();
            message_index++) {
            const Descriptor* message = &file->message_types_[message_index];
            auto* hints = FindOrNull(message_hints_, message);
            if (!hints)
                continue;
            constexpr int kMaxSuggestions = 3;
            int fields_to_suggest = std::min(kMaxSuggestions, hints->fields_to_suggest);
            if (fields_to_suggest <= 0)
                continue;
            struct Range {
                int from;
                int to;
            };
            std::vector<Range> used_ordinals;
            auto add_ordinal = [&](int ordinal) {
                if (ordinal <= 0 || ordinal > FieldDescriptor::kMaxNumber)
                    return;
                if (!used_ordinals.empty() && ordinal == used_ordinals.back().to) {
                    used_ordinals.back().to = ordinal + 1;
                } else {
                    used_ordinals.push_back({ ordinal, ordinal + 1 });
                }
            };
            auto add_range = [&](int from, int to) {
                from = std::max(0, std::min(FieldDescriptor::kMaxNumber + 1, from));
                to = std::max(0, std::min(FieldDescriptor::kMaxNumber + 1, to));
                if (from >= to)
                    return;
                used_ordinals.push_back({ from, to });
            };
            for (int i = 0; i < message->field_count(); i++) {
                add_ordinal(message->field(i)->number());
            }
            for (int i = 0; i < message->extension_count(); i++) {
                add_ordinal(message->extension(i)->number());
            }
            for (int i = 0; i < message->reserved_range_count(); i++) {
                auto range = message->reserved_range(i);
                add_range(range->start, range->end);
            }
            for (int i = 0; i < message->extension_range_count(); i++) {
                auto range = message->extension_range(i);
                add_range(range->start, range->end);
            }
            used_ordinals.push_back(
                { FieldDescriptor::kMaxNumber, FieldDescriptor::kMaxNumber + 1 });
            used_ordinals.push_back({ FieldDescriptor::kFirstReservedNumber,
                FieldDescriptor::kLastReservedNumber });
            std::sort(used_ordinals.begin(), used_ordinals.end(),
                [](Range lhs, Range rhs) {
                    return std::tie(lhs.from, lhs.to) < std::tie(rhs.from, rhs.to);
                });
            int current_ordinal = 1;
            std::stringstream id_list;
            id_list << "Suggested field numbers for " << message->full_name() << ": ";
            const char* separator = "";
            for (auto& current_range : used_ordinals) {
                while (current_ordinal < current_range.from && fields_to_suggest > 0) {
                    id_list << separator << current_ordinal++;
                    separator = ", ";
                    fields_to_suggest--;
                }
                if (fields_to_suggest == 0)
                    break;
                current_ordinal = std::max(current_ordinal, current_range.to);
            }
            if (hints->first_reason) {
                AddError(message->full_name(), *hints->first_reason,
                    hints->first_reason_location, id_list.str());
            }
        }
    }


#define VALIDATE_OPTIONS_FROM_ARRAY(descriptor, array_name, type) \
    for (int i = 0; i < descriptor->array_name##_count(); ++i) {  \
        Validate##type##Options(descriptor->array_name##s_ + i,   \
            proto.array_name(i));                                 \
    }

    // Determine if the file uses optimize_for = LITE_RUNTIME, being careful to
    // avoid problems that exist at init time.
    static bool IsLite(const FileDescriptor* file) {
        // TODO(kenton):  I don't even remember how many of these conditions are
        //   actually possible.  I'm just being super-safe.
        return file != nullptr && &file->options() != &FileOptions::default_instance() && file->options().optimize_for() == FileOptions_OptimizeMode::LITE_RUNTIME;
    }

    void DescriptorBuilder::ValidateFileOptions(FileDescriptor* file,
        const FileDescriptorProto& proto) {
        VALIDATE_OPTIONS_FROM_ARRAY(file, message_type, Message);
        VALIDATE_OPTIONS_FROM_ARRAY(file, enum_type, Enum);
        VALIDATE_OPTIONS_FROM_ARRAY(file, service, Service);
        VALIDATE_OPTIONS_FROM_ARRAY(file, extension, Field);

        // Lite files can only be imported by other Lite files.
        if (!IsLite(file)) {
            for (int i = 0; i < file->dependency_count(); i++) {
                if (IsLite(file->dependency(i))) {
                    AddError(
                        file->dependency(i)->name(), proto,
                        DescriptorPool::ErrorCollector::IMPORT,
                        "Files that do not use optimize_for = LITE_RUNTIME cannot import "
                        "files which do use this option.  This file is not lite, but it "
                        "imports \""
                            + file->dependency(i)->name() + "\" which is.");
                    break;
                }
            }
        }
    }

    void DescriptorBuilder::ValidateProto3(FileDescriptor* file,
        const FileDescriptorProto& proto) {
        for (int i = 0; i < file->extension_count(); ++i) {
            ValidateProto3Field(file->extensions_ + i, proto.extension(i));
        }
        for (int i = 0; i < file->message_type_count(); ++i) {
            ValidateProto3Message(file->message_types_ + i, proto.message_type(i));
        }
        for (int i = 0; i < file->enum_type_count(); ++i) {
            ValidateProto3Enum(file->enum_types_ + i, proto.enum_type(i));
        }
    }



    static std::string ToLowercaseWithoutUnderscores(const std::string& name) {
        std::string result;
        for (char character : name) {
            if (character != '_') {
                if (character >= 'A' && character <= 'Z') {
                    result.push_back(character - 'A' + 'a');
                } else {
                    result.push_back(character);
                }
            }
        }
        return result;
    }

    void DescriptorBuilder::ValidateProto3Message(Descriptor* message,
        const DescriptorProto& proto) {
        for (int i = 0; i < message->nested_type_count(); ++i) {
            ValidateProto3Message(message->nested_types_ + i, proto.nested_type(i));
        }
        for (int i = 0; i < message->enum_type_count(); ++i) {
            ValidateProto3Enum(message->enum_types_ + i, proto.enum_type(i));
        }
        for (int i = 0; i < message->field_count(); ++i) {
            ValidateProto3Field(message->fields_ + i, proto.field(i));
        }
        for (int i = 0; i < message->extension_count(); ++i) {
            ValidateProto3Field(message->extensions_ + i, proto.extension(i));
        }
        if (message->extension_range_count() > 0) {
            AddError(message->full_name(), proto.extension_range(0),
                DescriptorPool::ErrorCollector::NUMBER,
                "Extension ranges are not allowed in proto3.");
        }
        if (message->options().message_set_wire_format()) {
            // Using MessageSet doesn't make sense since we disallow extensions.
            AddError(message->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
                "MessageSet is not supported in proto3.");
        }

        // In proto3, we reject field names if they conflict in camelCase.
        // Note that we currently enforce a stricter rule: Field names must be
        // unique after being converted to lowercase with underscores removed.
        std::map<std::string, const FieldDescriptor*> name_to_field;
        for (int i = 0; i < message->field_count(); ++i) {
            std::string lowercase_name = ToLowercaseWithoutUnderscores(message->field(i)->name());
            if (name_to_field.find(lowercase_name) != name_to_field.end()) {
                AddError(message->full_name(), proto.field(i),
                    DescriptorPool::ErrorCollector::NAME,
                    "The JSON camel-case name of field \"" + message->field(i)->name() + "\" conflicts with field \"" + name_to_field[lowercase_name]->name() + "\". This is not " + "allowed in proto3.");
            } else {
                name_to_field[lowercase_name] = message->field(i);
            }
        }
    }


    namespace {

         std::set<std::string>* NewAllowedProto3Extendee() {
             auto allowed_proto3_extendees = new std::set<std::string>;
             const char* kOptionNames[] = {
                 "FileOptions", "MessageOptions", "FieldOptions",
                 "EnumOptions", "EnumValueOptions", "ServiceOptions",
                 "MethodOptions", "OneofOptions", "ExtensionRangeOptions"
             };
             for (const char* option_name : kOptionNames) {
                 // descriptor.shark has a different package name in opensource. We allow
                 // both so the opensource protocol compiler can also compile internal
                 // proto3 files with custom options. See: b/27567912
                 allowed_proto3_extendees->insert(std::string("shark.") + option_name);
                 // Split the word to trick the opensource processing scripts so they
                 // will keep the original package name.
                 allowed_proto3_extendees->insert(std::string("proto") + "2." + option_name);
             }
             return allowed_proto3_extendees;
         }

         // Checks whether the extendee type is allowed in proto3.
         // Only extensions to descriptor options are allowed. We use name comparison
         // instead of comparing the descriptor directly because the extensions may be
         // defined in a different pool.
         bool AllowedExtendeeInProto3(const std::string& name) {
             static auto allowed_proto3_extendees = internal::OnShutdownDelete(NewAllowedProto3Extendee());
             return allowed_proto3_extendees->find(name) != allowed_proto3_extendees->end();
         }
     } // anonymous namespace

    void DescriptorBuilder::ValidateProto3Field(FieldDescriptor* field,
        const FieldDescriptorProto& proto) {
        if (field->is_extension() && !AllowedExtendeeInProto3(field->containing_type()->full_name())) {
            AddError(field->full_name(), proto,
                DescriptorPool::ErrorCollector::EXTENDEE,
                "Extensions in proto3 are only allowed for defining options.");
        }
        if (field->is_required()) {
            AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "Required fields are not allowed in proto3.");
        }
        if (field->has_default_value()) {
            AddError(field->full_name(), proto,
                DescriptorPool::ErrorCollector::DEFAULT_VALUE,
                "Explicit default values are not allowed in proto3.");
        }
        if (field->cpp_type() == SharkFieldCppType::CPPTYPE_ENUM && field->enum_type() && field->enum_type()->file()->syntax() != SyntaxEdition::SYNTAX_PROTO3 && field->enum_type()->file()->syntax() != SyntaxEdition::SYNTAX_UNKNOWN) {
            // Proto3 messages can only use Proto3 enum types; otherwise we can't
            // guarantee that the default value is zero.
            AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "Enum type \"" + field->enum_type()->full_name() + "\" is not a proto3 enum, but is used in \"" + field->containing_type()->full_name() + "\" which is a proto3 message type.");
        }
    }

    void DescriptorBuilder::ValidateProto3Enum(EnumDescriptor* enm,
        const EnumDescriptorProto& proto) {
        if (enm->value_count() > 0 && enm->value(0)->number() != 0) {
            AddError(enm->full_name(), proto.value(0),
                DescriptorPool::ErrorCollector::NUMBER,
                "The first enum value must be zero in proto3.");
        }
    }

    void DescriptorBuilder::ValidateMessageOptions(Descriptor* message,
        const DescriptorProto& proto) {
        VALIDATE_OPTIONS_FROM_ARRAY(message, field, Field);
        VALIDATE_OPTIONS_FROM_ARRAY(message, nested_type, Message);
        VALIDATE_OPTIONS_FROM_ARRAY(message, enum_type, Enum);
        VALIDATE_OPTIONS_FROM_ARRAY(message, extension, Field);

        const int64_t max_extension_range = static_cast<int64_t>(message->options().message_set_wire_format()
                ? std::numeric_limits<int32_t>::max()
                : FieldDescriptor::kMaxNumber);
        for (int i = 0; i < message->extension_range_count(); ++i) {
            if (message->extension_range(i)->end > max_extension_range + 1) {
                AddError(message->full_name(), proto.extension_range(i),
                    DescriptorPool::ErrorCollector::NUMBER,
                    strings::Substitute("Extension numbers cannot be greater than $0.",
                        max_extension_range));
            }

            ValidateExtensionRangeOptions(message->full_name(),
                message->extension_ranges_ + i,
                proto.extension_range(i));
        }
    }

    void DescriptorBuilder::ValidateFieldOptions(
        FieldDescriptor* field, const FieldDescriptorProto& proto) {
        if (pool_->lazily_build_dependencies_ && (!field || !field->message_type())) {
            return;
        }
        // Only message type fields may be lazy.
        if (field->options().lazy() || field->options().unverified_lazy()) {
            if (field->type() != SharkFieldType::MESSAGE) {
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                    "[lazy = true] can only be specified for submessage fields.");
            }
        }

        // Only repeated primitive fields may be packed.
        if (field->options().packed() && !field->is_packable()) {
            AddError(
                field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "[packed = true] can only be specified for repeated primitive fields.");
        }

        // Note:  Default instance may not yet be initialized here, so we have to
        //   avoid reading from it.
        if (field->containing_type_ != nullptr && &field->containing_type()->options() != &MessageOptions::default_instance() && field->containing_type()->options().message_set_wire_format()) {
            if (field->is_extension()) {
                if (!field->is_optional() || field->type() != SharkFieldType::MESSAGE) {
                    AddError(field->full_name(), proto,
                        DescriptorPool::ErrorCollector::TYPE,
                        "Extensions of MessageSets must be optional messages.");
                }
            } else {
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
                    "MessageSets cannot have fields, only extensions.");
            }
        }

        // Lite extensions can only be of Lite types.
        if (IsLite(field->file()) && field->containing_type_ != nullptr && !IsLite(field->containing_type()->file())) {
            AddError(field->full_name(), proto,
                DescriptorPool::ErrorCollector::EXTENDEE,
                "Extensions to non-lite types can only be declared in non-lite "
                "files.  Note that you cannot extend a non-lite type to contain "
                "a lite type, but the reverse is allowed.");
        }

        // Validate map types.
        if (field->is_map()) {
            if (!ValidateMapEntry(field, proto)) {
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                    "map_entry should not be set explicitly. Use map<KeyType, "
                    "ValueType> instead.");
            }
        }

        ValidateJSType(field, proto);

        // json_name option is not allowed on extension fields. Note that the
        // json_name field in FieldDescriptorProto is always populated by protoc
        // when it sends descriptor data to plugins (calculated from field name if
        // the option is not explicitly set) so we can't rely on its presence to
        // determine whether the json_name option is set on the field. Here we
        // compare it against the default calculated json_name value and consider
        // the option set if they are different. This won't catch the case when
        // an user explicitly sets json_name to the default value, but should be
        // good enough to catch common misuses.
        if (field->is_extension() && (field->has_json_name() && field->json_name() != ToJsonName(field->name()))) {
            AddError(field->full_name(), proto,
                DescriptorPool::ErrorCollector::OPTION_NAME,
                "option json_name is not allowed on extension fields.");
        }
    }

    void DescriptorBuilder::ValidateEnumOptions(EnumDescriptor* enm,
        const EnumDescriptorProto& proto) {
        VALIDATE_OPTIONS_FROM_ARRAY(enm, value, EnumValue);
        if (!enm->options().has_allow_alias() || !enm->options().allow_alias()) {
            std::map<int, std::string> used_values;
            for (int i = 0; i < enm->value_count(); ++i) {
                const EnumValueDescriptor* enum_value = enm->value(i);
                if (used_values.find(enum_value->number()) != used_values.end()) {
                    std::string error = "\"" + enum_value->full_name() + "\" uses the same enum value as \"" + used_values[enum_value->number()] + "\". If this is intended, set "
                                                                                                                                                   "'option allow_alias = true;' to the enum definition.";
                    if (!enm->options().allow_alias()) {
                        // Generate error if duplicated enum values are explicitly disallowed.
                        AddError(enm->full_name(), proto.value(i),
                            DescriptorPool::ErrorCollector::NUMBER, error);
                    }
                } else {
                    used_values[enum_value->number()] = enum_value->full_name();
                }
            }
        }
    }



    void DescriptorBuilder::ValidateEnumValueOptions(
        EnumValueDescriptor* /* enum_value */,
        const EnumValueDescriptorProto& /* proto */) {
        // Nothing to do so far.
    }

    void DescriptorBuilder::ValidateExtensionRangeOptions(
        const std::string& full_name, Descriptor::ExtensionRange* extension_range,
        const DescriptorProto_ExtensionRange& proto) {
        (void)full_name; // Parameter is used by Google-internal code.
        (void)extension_range; // Parameter is used by Google-internal code.
    }

    void DescriptorBuilder::ValidateServiceOptions(
        ServiceDescriptor* service, const ServiceDescriptorProto& proto) {
        if (IsLite(service->file()) && service->file()->options().cc_generic_services()) {
            AddError(service->full_name(), proto, DescriptorPool::ErrorCollector::NAME,
                "Files with optimize_for = LITE_RUNTIME cannot define services "
                "unless you set cc_generic_services to false.");
        }

        VALIDATE_OPTIONS_FROM_ARRAY(service, method, Method);
    }

    void DescriptorBuilder::ValidateMethodOptions(
        MethodDescriptor* /* method */, const MethodDescriptorProto& /* proto */) {
        // Nothing to do so far.
    }

    bool DescriptorBuilder::ValidateMapEntry(FieldDescriptor* field,
        const FieldDescriptorProto& proto) {
        const Descriptor* message = field->message_type();
        if ( // Must not contain extensions, extension range or nested message or
             // enums
            message->extension_count() != 0 || field->label() != FieldLabel::REPEATED || message->extension_range_count() != 0 || message->nested_type_count() != 0 || message->enum_type_count() != 0 ||
            // Must contain exactly two fields
            message->field_count() != 2 ||
            // Field name and message name must match
            message->name() != ToCamelCase(field->name(), false) + "Entry" ||
            // Entry message must be in the same containing type of the field.
            field->containing_type() != message->containing_type()) {
            return false;
        }

        const FieldDescriptor* key = message->map_key();
        const FieldDescriptor* value = message->map_value();
        if (key->label() != FieldLabel::OPTIONAL || key->number() != 1 || key->name() != "key") {
            return false;
        }
        if (value->label() != FieldLabel::OPTIONAL || value->number() != 2 || value->name() != "value") {
            return false;
        }

        // Check key types are legal.
        switch (key->type()) {
        case SharkFieldType::ENUM:
            AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "Key in map fields cannot be enum types.");
            break;
        case SharkFieldType::FLOAT:
        case SharkFieldType::DOUBLE:
        case SharkFieldType::MESSAGE:
        case SharkFieldType::BYTES:
            AddError(
                field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "Key in map fields cannot be float/double, bytes or message types.");
            break;
        case SharkFieldType::BOOL:
        case SharkFieldType::INT32:
        case SharkFieldType::INT8:
        case SharkFieldType::INT16:
        case SharkFieldType::INT64:
        case SharkFieldType::SINT32:
        case SharkFieldType::SINT64:
        case SharkFieldType::STRING:
        case SharkFieldType::UINT32:
        case SharkFieldType::UINT8:
        case SharkFieldType::UINT16:
        case SharkFieldType::UINT64:
        case SharkFieldType::FIXED32:
        case SharkFieldType::FIXED64:
        case SharkFieldType::SFIXED32:
        case SharkFieldType::SFIXED64:
            // Legal cases
            break;
            // Do not add a default, so that the compiler will complain when new types
            // are added.
        }

        if (value->type() == SharkFieldType::ENUM) {
            if (value->enum_type()->value(0)->number() != 0) {
                AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                    "Enum value in map must define 0 as the first value.");
            }
        }

        return true;
    }

    void DescriptorBuilder::DetectMapConflicts(const Descriptor* message,
        const DescriptorProto& proto) {
        std::map<std::string, const Descriptor*> seen_types;
        for (int i = 0; i < message->nested_type_count(); ++i) {
            const Descriptor* nested = message->nested_type(i);
            std::pair<std::map<std::string, const Descriptor*>::iterator, bool> result = seen_types.insert(std::make_pair(nested->name(), nested));
            if (!result.second) {
                if (result.first->second->options().map_entry() || nested->options().map_entry()) {
                    AddError(message->full_name(), proto,
                        DescriptorPool::ErrorCollector::NAME,
                        "Expanded map entry type " + nested->name() + " conflicts with an existing nested message type.");
                    break;
                }
            }
            // Recursively test on the nested types.
            DetectMapConflicts(message->nested_type(i), proto.nested_type(i));
        }
        // Check for conflicted field names.
        for (int i = 0; i < message->field_count(); ++i) {
            const FieldDescriptor* field = message->field(i);
            std::map<std::string, const Descriptor*>::iterator iter = seen_types.find(field->name());
            if (iter != seen_types.end() && iter->second->options().map_entry()) {
                AddError(message->full_name(), proto,
                    DescriptorPool::ErrorCollector::NAME,
                    "Expanded map entry type " + iter->second->name() + " conflicts with an existing field.");
            }
        }
        // Check for conflicted enum names.
        for (int i = 0; i < message->enum_type_count(); ++i) {
            const EnumDescriptor* enum_desc = message->enum_type(i);
            std::map<std::string, const Descriptor*>::iterator iter = seen_types.find(enum_desc->name());
            if (iter != seen_types.end() && iter->second->options().map_entry()) {
                AddError(message->full_name(), proto,
                    DescriptorPool::ErrorCollector::NAME,
                    "Expanded map entry type " + iter->second->name() + " conflicts with an existing enum type.");
            }
        }
        // Check for conflicted oneof names.
        for (int i = 0; i < message->oneof_decl_count(); ++i) {
            const OneofDescriptor* oneof_desc = message->oneof_decl(i);
            std::map<std::string, const Descriptor*>::iterator iter = seen_types.find(oneof_desc->name());
            if (iter != seen_types.end() && iter->second->options().map_entry()) {
                AddError(message->full_name(), proto,
                    DescriptorPool::ErrorCollector::NAME,
                    "Expanded map entry type " + iter->second->name() + " conflicts with an existing oneof type.");
            }
        }
    }

    void DescriptorBuilder::ValidateJSType(FieldDescriptor* field,
        const FieldDescriptorProto& proto) {
        FieldOptions::JSType jstype = field->options().jstype();
        // The default is always acceptable.
        if (jstype == FieldOptions::JSType::JS_NORMAL) {
            return;
        }

        switch (field->type()) {
        // Integral 64-bit types may be represented as JavaScript numbers or
        // strings.
        case SharkFieldType::UINT64:
        case SharkFieldType::INT64:
        case SharkFieldType::SINT64:
        case SharkFieldType::FIXED64:
        case SharkFieldType::SFIXED64:
            if (jstype == FieldOptions::JSType::JS_STRING || jstype == FieldOptions::JSType::JS_NUMBER) {
                return;
            }
            AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "Illegal jstype for int64, uint64, sint64, fixed64 "
                "or sfixed64 field: "
                    + FieldOptions_JSTypeTrait::descriptor()
                        ->value(FieldOptions_JSTypeTrait::to_underlying(jstype))
                        ->name());
            break;

        // No other types permit a jstype option.
        default:
            AddError(field->full_name(), proto, DescriptorPool::ErrorCollector::TYPE,
                "jstype is only allowed on int64, uint64, sint64, fixed64 "
                "or sfixed64 fields.");
            break;
        }
    }

#undef VALIDATE_OPTIONS_FROM_ARRAY

    // -------------------------------------------------------------------

    DescriptorBuilder::OptionInterpreter::OptionInterpreter(
        DescriptorBuilder* builder)
        : builder_(builder) {
        GOOGLE_CHECK(builder_);
    }


    // A custom error collector to record any text-format parsing errors
    namespace {
        class AggregateErrorCollector : public io::ErrorCollector {
        public:
            std::string error_;

            void AddError(int /* line */, int /* column */,
                const std::string& message) override {
                if (!error_.empty()) {
                    error_ += "; ";
                }
                error_ += message;
            }

            void AddWarning(int /* line */, int /* column */,
                const std::string& /* message */) override {
                // Ignore warnings
            }
        };
    } // namespace

    // We construct a dynamic message of the type corresponding to
    // option_field, parse the supplied text-format string into this
    // message, and serialize the resulting message to produce the value.
    bool DescriptorBuilder::OptionInterpreter::SetAggregateOption(
        const FieldDescriptor* option_field, UnknownFieldSet* unknown_fields) {
        if (!uninterpreted_option_->has_aggregate_value()) {
            return AddValueError("Option \"" + option_field->full_name() + "\" is a message. To set the entire message, use "
                                                                           "syntax like \""
                + option_field->name() + " = { <proto text format> }\". "
                                         "To set fields within it, use "
                                         "syntax like \""
                + option_field->name() + ".foo = value\".");
        }

        const Descriptor* type = option_field->message_type();
        std::unique_ptr<Message> dynamic(dynamic_factory_.GetPrototype(type)->New());
        GOOGLE_CHECK(dynamic.get() != nullptr)
            << "Could not create an instance of " << option_field->DebugString();

        AggregateErrorCollector collector;
        AggregateOptionFinder finder;
        finder.builder_ = builder_;
        TextFormat::Parser parser;
        parser.RecordErrorsTo(&collector);
        parser.SetFinder(&finder);
        if (!parser.ParseFromString(uninterpreted_option_->aggregate_value(),
                dynamic.get())) {
            AddValueError("Error while parsing option value for \"" + option_field->name() + "\": " + collector.error_);
            return false;
        } else {
            std::string serial;
            dynamic->SerializeToString(&serial); // Never fails
            unknown_fields->AddLengthDelimited(option_field->number(), serial);
            return true;
        }
    }

    void DescriptorBuilder::OptionInterpreter::SetInt32(
        int number, int32_t value, SharkFieldType type,
        UnknownFieldSet* unknown_fields) {
        switch (type) {
        case SharkFieldType::INT32:
        case SharkFieldType::INT8:
        case SharkFieldType::INT16:
            unknown_fields->AddVarint(
                number, static_cast<uint64_t>(static_cast<int64_t>(value)));
            break;

        case SharkFieldType::SFIXED32:
            unknown_fields->AddFixed32(number, static_cast<uint32_t>(value));
            break;

        case SharkFieldType::SINT32:
            unknown_fields->AddVarint(
                number, internal::WireFormatLite::ZigZagEncode32(value));
            break;

        default:
            GOOGLE_LOG(FATAL) << "Invalid wire type for SharkFieldCppType::CPPTYPE_INT32: " << static_cast<int>(type);
            break;
        }
    }

    void DescriptorBuilder::OptionInterpreter::SetInt64(
        int number, int64_t value, SharkFieldType type,
        UnknownFieldSet* unknown_fields) {
        switch (type) {
        case SharkFieldType::INT64:
            unknown_fields->AddVarint(number, static_cast<uint64_t>(value));
            break;

        case SharkFieldType::SFIXED64:
            unknown_fields->AddFixed64(number, static_cast<uint64_t>(value));
            break;

        case SharkFieldType::SINT64:
            unknown_fields->AddVarint(
                number, internal::WireFormatLite::ZigZagEncode64(value));
            break;

        default:
            GOOGLE_LOG(FATAL) << "Invalid wire type for SharkFieldCppType::CPPTYPE_INT64: " << static_cast<int>(type);
            break;
        }
    }

    void DescriptorBuilder::OptionInterpreter::SetUInt32(
        int number, uint32_t value, SharkFieldType type,
        UnknownFieldSet* unknown_fields) {
        switch (type) {
        case SharkFieldType::UINT32:
        case SharkFieldType::UINT8:
        case SharkFieldType::UINT16:
            unknown_fields->AddVarint(number, static_cast<uint64_t>(value));
            break;

        case SharkFieldType::FIXED32:
            unknown_fields->AddFixed32(number, static_cast<uint32_t>(value));
            break;

        default:
            GOOGLE_LOG(FATAL) << "Invalid wire type for SharkFieldCppType::CPPTYPE_UINT32: " << static_cast<int>(type);
            break;
        }
    }

    void DescriptorBuilder::OptionInterpreter::SetUInt64(
        int number, uint64_t value, SharkFieldType type,
        UnknownFieldSet* unknown_fields) {
        switch (type) {
        case SharkFieldType::UINT64:
            unknown_fields->AddVarint(number, value);
            break;

        case SharkFieldType::FIXED64:
            unknown_fields->AddFixed64(number, value);
            break;

        default:
            GOOGLE_LOG(FATAL) << "Invalid wire type for SharkFieldCppType::CPPTYPE_UINT64: " << static_cast<int>(type);
            break;
        }
    }

    void DescriptorBuilder::LogUnusedDependency(const FileDescriptorProto& proto,
        const FileDescriptor* result) {
        (void)result; // Parameter is used by Google-internal code.

        if (!unused_dependency_.empty()) {
            auto itr = pool_->unused_import_track_files_.find(proto.name());
            bool is_error = itr != pool_->unused_import_track_files_.end() && itr->second;
            for (std::set<const FileDescriptor*>::const_iterator it = unused_dependency_.begin();
                it != unused_dependency_.end(); ++it) {
                std::string error_message = "Import " + (*it)->name() + " is unused.";
                if (is_error) {
                    AddError((*it)->name(), proto, DescriptorPool::ErrorCollector::IMPORT,
                        error_message);
                } else {
                    AddWarning((*it)->name(), proto, DescriptorPool::ErrorCollector::IMPORT,
                        error_message);
                }
            }
        }
    }

} // namespace shark


#include <shark/port_undef.inc>
