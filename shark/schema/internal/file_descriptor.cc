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
#include <shark/schema/internal/field_descriptor.h>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    const char* FileDescriptor::SyntaxName(SyntaxEdition syntax) {
        switch (syntax) {
        case SyntaxEdition::SYNTAX_PROTO2:
            return "proto2";
        case SyntaxEdition::SYNTAX_PROTO3:
            return "proto3";
        case SyntaxEdition::SYNTAX_UNKNOWN:
            return "unknown";
        }
        GOOGLE_LOG(FATAL) << "can't reach here.";
        return nullptr;
    }

    const Descriptor* FileDescriptor::FindMessageTypeByName(
    ConstStringParam key) const {
        return tables_->FindNestedSymbol(this, key).descriptor();
    }

    const EnumDescriptor* FileDescriptor::FindEnumTypeByName(
        ConstStringParam key) const {
        return tables_->FindNestedSymbol(this, key).enum_descriptor();
    }

    const EnumValueDescriptor* FileDescriptor::FindEnumValueByName(
        ConstStringParam key) const {
        return tables_->FindNestedSymbol(this, key).enum_value_descriptor();
    }

    const ServiceDescriptor* FileDescriptor::FindServiceByName(
        ConstStringParam key) const {
        return tables_->FindNestedSymbol(this, key).service_descriptor();
    }

    const FieldDescriptor* FileDescriptor::FindExtensionByName(
        ConstStringParam key) const {
        const FieldDescriptor* field = tables_->FindNestedSymbol(this, key).field_descriptor();
        return field != nullptr && field->is_extension() ? field : nullptr;
    }

    const FieldDescriptor* FileDescriptor::FindExtensionByLowercaseName(
        ConstStringParam key) const {
        const FieldDescriptor* result = tables_->FindFieldByLowercaseName(this, key);
        if (result == nullptr || !result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

    const FieldDescriptor* FileDescriptor::FindExtensionByCamelcaseName(
        ConstStringParam key) const {
        const FieldDescriptor* result = tables_->FindFieldByCamelcaseName(this, key);
        if (result == nullptr || !result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

     bool FieldDescriptor::is_map_message_type() const {
        return type_descriptor_.message_type->options().map_entry();
    }

    std::string FieldDescriptor::DefaultValueAsString(
        bool quote_string_type) const {
        GOOGLE_CHECK(has_default_value()) << "No default value";
        switch (cpp_type()) {
        case SharkFieldCppType::CPPTYPE_INT32:
        case SharkFieldCppType::CPPTYPE_INT8:
        case SharkFieldCppType::CPPTYPE_INT16:
            return StrCat(default_value_int32_t());
        case SharkFieldCppType::CPPTYPE_UINT8:
        case SharkFieldCppType::CPPTYPE_UINT16:
            return StrCat(default_value_uint32_t());
        case SharkFieldCppType::CPPTYPE_INT64:
            return StrCat(default_value_int64_t());
        case SharkFieldCppType::CPPTYPE_UINT32:
            return StrCat(default_value_uint32_t());
        case SharkFieldCppType::CPPTYPE_UINT64:
            return StrCat(default_value_uint64_t());
        case SharkFieldCppType::CPPTYPE_FLOAT:
            return SimpleFtoa(default_value_float());
        case SharkFieldCppType::CPPTYPE_DOUBLE:
            return SimpleDtoa(default_value_double());
        case SharkFieldCppType::CPPTYPE_BOOL:
            return default_value_bool() ? "true" : "false";
        case SharkFieldCppType::CPPTYPE_STRING:
            if (quote_string_type) {
                return "\"" + CEscape(default_value_string()) + "\"";
            } else {
                if (type() == SharkFieldType::BYTES) {
                    return CEscape(default_value_string());
                } else {
                    return default_value_string();
                }
            }
        case SharkFieldCppType::CPPTYPE_ENUM:
            return default_value_enum()->name();
        case SharkFieldCppType::CPPTYPE_MESSAGE:
            GOOGLE_LOG(DFATAL) << "Messages can't have default values!";
            break;
        }
        GOOGLE_LOG(FATAL) << "Can't get here: failed to get default value as string";
        return "";
    }

    // CopyTo methods ====================================================

    void FileDescriptor::CopyTo(FileDescriptorProto* proto) const {
        proto->set_name(name());
        if (!package().empty())
            proto->set_package(package());
        // TODO(liujisi): Also populate when syntax="proto2".
        if (syntax() == SyntaxEdition::SYNTAX_PROTO3)
            proto->set_syntax(SyntaxName(syntax()));

        for (int i = 0; i < dependency_count(); i++) {
            proto->add_dependency(dependency(i)->name());
        }

        for (int i = 0; i < public_dependency_count(); i++) {
            proto->add_public_dependency(public_dependencies_[i]);
        }

        for (int i = 0; i < weak_dependency_count(); i++) {
            proto->add_weak_dependency(weak_dependencies_[i]);
        }

        for (int i = 0; i < message_type_count(); i++) {
            message_type(i)->CopyTo(proto->add_message_type());
        }
        for (int i = 0; i < enum_type_count(); i++) {
            enum_type(i)->CopyTo(proto->add_enum_type());
        }
        for (int i = 0; i < service_count(); i++) {
            service(i)->CopyTo(proto->add_service());
        }
        for (int i = 0; i < extension_count(); i++) {
            extension(i)->CopyTo(proto->add_extension());
        }

        if (&options() != &FileOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

    void FileDescriptor::CopyJsonNameTo(FileDescriptorProto* proto) const {
        if (message_type_count() != proto->message_type_size() || extension_count() != proto->extension_size()) {
            GOOGLE_LOG(ERROR) << "Cannot copy json_name to a proto of a different size.";
            return;
        }
        for (int i = 0; i < message_type_count(); i++) {
            message_type(i)->CopyJsonNameTo(proto->mutable_message_type(i));
        }
        for (int i = 0; i < extension_count(); i++) {
            extension(i)->CopyJsonNameTo(proto->mutable_extension(i));
        }
    }

    void FileDescriptor::CopySourceCodeInfoTo(FileDescriptorProto* proto) const {
        if (source_code_info_ && source_code_info_ != &SourceCodeInfo::default_instance()) {
            proto->mutable_source_code_info()->CopyFrom(*source_code_info_);
        }
    }

     std::string FileDescriptor::DebugString() const {
        DebugStringOptions options; // default options
        return DebugStringWithOptions(options);
    }

    std::string FileDescriptor::DebugStringWithOptions(
        const DebugStringOptions& debug_string_options) const {
        std::string contents;
        {
            std::vector<int> path;
            path.push_back(FileDescriptorProto::kSyntaxFieldNumber);
            SourceLocationCommentPrinter syntax_comment(this, path, "",
                debug_string_options);
            syntax_comment.AddPreComment(&contents);
            strings::SubstituteAndAppend(&contents, "syntax = \"$0\";\n\n",
                SyntaxName(syntax()));
            syntax_comment.AddPostComment(&contents);
        }

        SourceLocationCommentPrinter comment_printer(this, "", debug_string_options);
        comment_printer.AddPreComment(&contents);

        std::set<int> public_dependencies;
        std::set<int> weak_dependencies;
        public_dependencies.insert(public_dependencies_,
            public_dependencies_ + public_dependency_count_);
        weak_dependencies.insert(weak_dependencies_,
            weak_dependencies_ + weak_dependency_count_);

        for (int i = 0; i < dependency_count(); i++) {
            if (public_dependencies.count(i) > 0) {
                strings::SubstituteAndAppend(&contents, "import public \"$0\";\n",
                    dependency(i)->name());
            } else if (weak_dependencies.count(i) > 0) {
                strings::SubstituteAndAppend(&contents, "import weak \"$0\";\n",
                    dependency(i)->name());
            } else {
                strings::SubstituteAndAppend(&contents, "import \"$0\";\n",
                    dependency(i)->name());
            }
        }

        if (!package().empty()) {
            std::vector<int> path;
            path.push_back(FileDescriptorProto::kPackageFieldNumber);
            SourceLocationCommentPrinter package_comment(this, path, "",
                debug_string_options);
            package_comment.AddPreComment(&contents);
            strings::SubstituteAndAppend(&contents, "package $0;\n\n", package());
            package_comment.AddPostComment(&contents);
        }

        if (FormatLineOptions(0, options(), pool(), &contents)) {
            contents.append("\n"); // add some space if we had options
        }

        for (int i = 0; i < enum_type_count(); i++) {
            enum_type(i)->DebugString(0, &contents, debug_string_options);
            contents.append("\n");
        }

        for (int i = 0; i < message_type_count(); i++) {
            message_type(i)->DebugString(0, &contents, debug_string_options,
                /* include_opening_clause */ true);
            contents.append("\n");
        }

        for (int i = 0; i < service_count(); i++) {
            service(i)->DebugString(&contents, debug_string_options);
            contents.append("\n");
        }

        const Descriptor* containing_type = nullptr;
        for (int i = 0; i < extension_count(); i++) {
            if (extension(i)->containing_type() != containing_type) {
                if (i > 0)
                    contents.append("}\n\n");
                containing_type = extension(i)->containing_type();
                strings::SubstituteAndAppend(&contents, "extend .$0 {\n",
                    containing_type->full_name());
            }
            extension(i)->DebugString(1, &contents, debug_string_options);
        }
        if (extension_count() > 0)
            contents.append("}\n\n");

        comment_printer.AddPostComment(&contents);

        return contents;
    }

    bool FileDescriptor::GetSourceLocation(const std::vector<int>& path,
       SourceLocation* out_location) const {
        GOOGLE_CHECK(out_location != nullptr);
        if (source_code_info_) {
            if (const SourceCodeInfo_Location* loc = tables_->GetSourceLocation(path, source_code_info_)) {
                const RepeatedField<int32_t>& span = loc->span();
                if (span.size() == 3 || span.size() == 4) {
                    out_location->start_line = span.Get(0);
                    out_location->start_column = span.Get(1);
                    out_location->end_line = span.Get(span.size() == 3 ? 0 : 2);
                    out_location->end_column = span.Get(span.size() - 1);

                    out_location->leading_comments = loc->leading_comments();
                    out_location->trailing_comments = loc->trailing_comments();
                    out_location->leading_detached_comments.assign(
                        loc->leading_detached_comments().begin(),
                        loc->leading_detached_comments().end());
                    return true;
                }
            }
        }
        return false;
    }

    bool FileDescriptor::GetSourceLocation(SourceLocation* out_location) const {
        std::vector<int> path; // empty path for root FileDescriptor
        return GetSourceLocation(path, out_location);
    }

    bool FieldDescriptor::is_packed() const {
        if (!is_packable())
            return false;
        return (options_ != nullptr) && options_->packed();
    }


    // Handle the lazy import building for a message field whose type wasn't built
    // at cross link time. If that was the case, we saved the name of the type to
    // be looked up when the accessor for the type was called. Set type_,
    // enum_type_, message_type_, and default_value_enum_ appropriately.
    void FieldDescriptor::InternalTypeOnceInit() const {
        GOOGLE_CHECK(file()->finished_building_ == true);
        const EnumDescriptor* enum_type = nullptr;
        const char* lazy_type_name = reinterpret_cast<const char*>(type_once_ + 1);
        const char* lazy_default_value_enum_name = lazy_type_name + strlen(lazy_type_name) + 1;
        Symbol result = file()->pool()->CrossLinkOnDemandHelper(
            lazy_type_name, static_cast<SharkFieldType>(type_) == SharkFieldType::ENUM);
        if (result.type() == Symbol::MESSAGE) {
            type_ = static_cast<uint8_t>(SharkFieldType::MESSAGE);
            type_descriptor_.message_type = result.descriptor();
        } else if (result.type() == Symbol::ENUM) {
            type_ = static_cast<uint8_t>(SharkFieldType::ENUM);
            enum_type = type_descriptor_.enum_type = result.enum_descriptor();
        }

        if (enum_type) {
            if (lazy_default_value_enum_name[0] != '\0') {
                // Have to build the full name now instead of at CrossLink time,
                // because enum_type may not be known at the time.
                // Enum values are scoped to their enum type (enum class rules):
                // the value's full name is the enum's full name plus the value
                // name, matching BuildEnumValue and the non-lazy default-value
                // lookup path in CrossLinkField.
                std::string name = enum_type->full_name();
                name += ".";
                name += lazy_default_value_enum_name;
                Symbol result = file()->pool()->CrossLinkOnDemandHelper(name, true);
                default_value_enum_ = result.enum_value_descriptor();
            } else {
                default_value_enum_ = nullptr;
            }
            if (!default_value_enum_) {
                // We use the first defined value as the default
                // if a default is not explicitly defined.
                GOOGLE_CHECK(enum_type->value_count());
                default_value_enum_ = enum_type->value(0);
            }
        }
    }

    void FieldDescriptor::TypeOnceInit(const FieldDescriptor* to_init) {
        to_init->InternalTypeOnceInit();
    }

    // message_type(), enum_type(), default_value_enum(), and type()
    // all share the same internal::call_once init path to do lazy
    // import building and cross linking of a field of a message.
    const Descriptor* FieldDescriptor::message_type() const {
        if (type_once_) {
            internal::call_once(*type_once_, FieldDescriptor::TypeOnceInit, this);
        }
        return static_cast<SharkFieldType>(type_) == SharkFieldType::MESSAGE
            ? type_descriptor_.message_type
            : nullptr;
    }

    const EnumDescriptor* FieldDescriptor::enum_type() const {
        if (type_once_) {
            internal::call_once(*type_once_, FieldDescriptor::TypeOnceInit, this);
        }
        return static_cast<SharkFieldType>(type_) == SharkFieldType::ENUM ? type_descriptor_.enum_type : nullptr;
    }


    const EnumValueDescriptor* FieldDescriptor::default_value_enum() const {
        if (type_once_) {
            internal::call_once(*type_once_, FieldDescriptor::TypeOnceInit, this);
        }
        return default_value_enum_;
    }

    const std::string& FieldDescriptor::PrintableNameForExtension() const {
        const bool is_message_set_extension = is_extension() && containing_type()->options().message_set_wire_format() && type() == SharkFieldType::MESSAGE && is_optional() && extension_scope() == message_type();
        return is_message_set_extension ? message_type()->full_name() : full_name();
    }

    void FileDescriptor::InternalDependenciesOnceInit() const {
        GOOGLE_CHECK(finished_building_ == true);
        const char* names_ptr = reinterpret_cast<const char*>(dependencies_once_ + 1);
        for (int i = 0; i < dependency_count(); i++) {
            const char* name = names_ptr;
            names_ptr += strlen(name) + 1;
            if (name[0] != '\0') {
                dependencies_[i] = pool_->FindFileByName(name);
            }
        }
    }

    void FileDescriptor::DependenciesOnceInit(const FileDescriptor* to_init) {
        to_init->InternalDependenciesOnceInit();
    }

    const FileDescriptor* FileDescriptor::dependency(int index) const {
        if (dependencies_once_) {
            // Do once init for all indices, as it's unlikely only a single index would
            // be called, and saves on internal::call_once allocations.
            internal::call_once(*dependencies_once_,
                FileDescriptor::DependenciesOnceInit, this);
        }
        return dependencies_[index];
    }

} // namespace shark


#include <shark/port_undef.inc>
