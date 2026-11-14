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

#include <shark/schema/internal/descriptor.h>
#include <shark/schema/internal/file_descriptor.h>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    const FieldDescriptor* Descriptor::FindFieldByNumber(int key) const {
        const FieldDescriptor* result = file()->tables_->FindFieldByNumber(this, key);
        if (result == nullptr || result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

     const FieldDescriptor* Descriptor::FindFieldByLowercaseName(
        ConstStringParam key) const {
        const FieldDescriptor* result = file()->tables_->FindFieldByLowercaseName(this, key);
        if (result == nullptr || result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

    const FieldDescriptor* Descriptor::FindFieldByCamelcaseName(
        ConstStringParam key) const {
        const FieldDescriptor* result = file()->tables_->FindFieldByCamelcaseName(this, key);
        if (result == nullptr || result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

    const FieldDescriptor* Descriptor::FindFieldByName(ConstStringParam key) const {
        const FieldDescriptor* field = file()->tables_->FindNestedSymbol(this, key).field_descriptor();
        return field != nullptr && !field->is_extension() ? field : nullptr;
    }

    const OneofDescriptor* Descriptor::FindOneofByName(ConstStringParam key) const {
        return file()->tables_->FindNestedSymbol(this, key).oneof_descriptor();
    }

    const FieldDescriptor* Descriptor::FindExtensionByName(
        ConstStringParam key) const {
        const FieldDescriptor* field = file()->tables_->FindNestedSymbol(this, key).field_descriptor();
        return field != nullptr && field->is_extension() ? field : nullptr;
    }

    const FieldDescriptor* Descriptor::FindExtensionByLowercaseName(
        ConstStringParam key) const {
        const FieldDescriptor* result = file()->tables_->FindFieldByLowercaseName(this, key);
        if (result == nullptr || !result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

    const FieldDescriptor* Descriptor::FindExtensionByCamelcaseName(
        ConstStringParam key) const {
        const FieldDescriptor* result = file()->tables_->FindFieldByCamelcaseName(this, key);
        if (result == nullptr || !result->is_extension()) {
            return nullptr;
        } else {
            return result;
        }
    }

    const Descriptor* Descriptor::FindNestedTypeByName(ConstStringParam key) const {
        return file()->tables_->FindNestedSymbol(this, key).descriptor();
    }

    const EnumDescriptor* Descriptor::FindEnumTypeByName(
        ConstStringParam key) const {
        return file()->tables_->FindNestedSymbol(this, key).enum_descriptor();
    }

    const EnumValueDescriptor* Descriptor::FindEnumValueByName(
        ConstStringParam key) const {
        return file()->tables_->FindNestedSymbol(this, key).enum_value_descriptor();
    }

    const FieldDescriptor* Descriptor::map_key() const {
        if (!options().map_entry())
            return nullptr;
        GOOGLE_DCHECK_EQ(field_count(), 2);
        return field(0);
    }

    const FieldDescriptor* Descriptor::map_value() const {
        if (!options().map_entry())
            return nullptr;
        GOOGLE_DCHECK_EQ(field_count(), 2);
        return field(1);
    }

    void Descriptor::ExtensionRange::CopyTo(
        DescriptorProto_ExtensionRange* proto) const {
        proto->set_start(this->start);
        proto->set_end(this->end);
        if (options_ != &ExtensionRangeOptions::default_instance()) {
            *proto->mutable_options() = *options_;
        }
    }

    const Descriptor::ExtensionRange*
    Descriptor::FindExtensionRangeContainingNumber(int number) const {
        // Linear search should be fine because we don't expect a message to have
        // more than a couple extension ranges.
        for (int i = 0; i < extension_range_count(); i++) {
            if (number >= extension_range(i)->start && number < extension_range(i)->end) {
                return extension_range(i);
            }
        }
        return nullptr;
    }

    const Descriptor::ReservedRange* Descriptor::FindReservedRangeContainingNumber(
        int number) const {
        // TODO(chrisn): Consider a non-linear search.
        for (int i = 0; i < reserved_range_count(); i++) {
            if (number >= reserved_range(i)->start && number < reserved_range(i)->end) {
                return reserved_range(i);
            }
        }
        return nullptr;
    }

    const EnumDescriptor::ReservedRange*
    EnumDescriptor::FindReservedRangeContainingNumber(int number) const {
        // TODO(chrisn): Consider a non-linear search.
        for (int i = 0; i < reserved_range_count(); i++) {
            if (number >= reserved_range(i)->start && number <= reserved_range(i)->end) {
                return reserved_range(i);
            }
        }
        return nullptr;
    }

    void Descriptor::CopyTo(DescriptorProto* proto) const {
        proto->set_name(name());

        for (int i = 0; i < field_count(); i++) {
            field(i)->CopyTo(proto->add_field());
        }
        for (int i = 0; i < oneof_decl_count(); i++) {
            oneof_decl(i)->CopyTo(proto->add_oneof_decl());
        }
        for (int i = 0; i < nested_type_count(); i++) {
            nested_type(i)->CopyTo(proto->add_nested_type());
        }
        for (int i = 0; i < enum_type_count(); i++) {
            enum_type(i)->CopyTo(proto->add_enum_type());
        }
        for (int i = 0; i < extension_range_count(); i++) {
            extension_range(i)->CopyTo(proto->add_extension_range());
        }
        for (int i = 0; i < extension_count(); i++) {
            extension(i)->CopyTo(proto->add_extension());
        }
        for (int i = 0; i < reserved_range_count(); i++) {
            DescriptorProto::ReservedRange* range = proto->add_reserved_range();
            range->set_start(reserved_range(i)->start);
            range->set_end(reserved_range(i)->end);
        }
        for (int i = 0; i < reserved_name_count(); i++) {
            proto->add_reserved_name(reserved_name(i));
        }

        if (&options() != &MessageOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

    void Descriptor::CopyJsonNameTo(DescriptorProto* proto) const {
        if (field_count() != proto->field_size() || nested_type_count() != proto->nested_type_size() || extension_count() != proto->extension_size()) {
            GOOGLE_LOG(ERROR) << "Cannot copy json_name to a proto of a different size.";
            return;
        }
        for (int i = 0; i < field_count(); i++) {
            field(i)->CopyJsonNameTo(proto->mutable_field(i));
        }
        for (int i = 0; i < nested_type_count(); i++) {
            nested_type(i)->CopyJsonNameTo(proto->mutable_nested_type(i));
        }
        for (int i = 0; i < extension_count(); i++) {
            extension(i)->CopyJsonNameTo(proto->mutable_extension(i));
        }
    }

    std::string Descriptor::DebugString() const {
        DebugStringOptions options; // default options
        return DebugStringWithOptions(options);
    }

    std::string Descriptor::DebugStringWithOptions(
        const DebugStringOptions& options) const {
        std::string contents;
        DebugString(0, &contents, options, /* include_opening_clause */ true);
        return contents;
    }

    void Descriptor::DebugString(int depth, std::string* contents,
        const DebugStringOptions& debug_string_options,
        bool include_opening_clause) const {
        if (options().map_entry()) {
            // Do not generate debug string for auto-generated map-entry type.
            return;
        }
        std::string prefix(depth * 2, ' ');
        ++depth;

        SourceLocationCommentPrinter comment_printer(this, prefix,
            debug_string_options);
        comment_printer.AddPreComment(contents);

        if (include_opening_clause) {
            strings::SubstituteAndAppend(contents, "$0message $1", prefix, name());
        }
        contents->append(" {\n");

        FormatLineOptions(depth, options(), file()->pool(), contents);

        for (int i = 0; i < nested_type_count(); i++) {
            nested_type(i)->DebugString(depth, contents, debug_string_options,
                /* include_opening_clause */ true);
        }
        for (int i = 0; i < enum_type_count(); i++) {
            enum_type(i)->DebugString(depth, contents, debug_string_options);
        }
        for (int i = 0; i < field_count(); i++) {
            if (field(i)->real_containing_oneof() == nullptr) {
                field(i)->DebugString(depth, contents, debug_string_options);
            } else if (field(i)->containing_oneof()->field(0) == field(i)) {
                // This is the first field in this oneof, so print the whole oneof.
                field(i)->containing_oneof()->DebugString(depth, contents,
                    debug_string_options);
            }
        }

        for (int i = 0; i < extension_range_count(); i++) {
            strings::SubstituteAndAppend(contents, "$0  extensions $1 to $2;\n", prefix,
                extension_range(i)->start,
                extension_range(i)->end - 1);
        }

        // Group extensions by what they extend, so they can be printed out together.
        const Descriptor* containing_type = nullptr;
        for (int i = 0; i < extension_count(); i++) {
            if (extension(i)->containing_type() != containing_type) {
                if (i > 0)
                    strings::SubstituteAndAppend(contents, "$0  }\n", prefix);
                containing_type = extension(i)->containing_type();
                strings::SubstituteAndAppend(contents, "$0  extend .$1 {\n", prefix,
                    containing_type->full_name());
            }
            extension(i)->DebugString(depth + 1, contents, debug_string_options);
        }
        if (extension_count() > 0)
            strings::SubstituteAndAppend(contents, "$0  }\n", prefix);

        if (reserved_range_count() > 0) {
            strings::SubstituteAndAppend(contents, "$0  reserved ", prefix);
            for (int i = 0; i < reserved_range_count(); i++) {
                const Descriptor::ReservedRange* range = reserved_range(i);
                if (range->end == range->start + 1) {
                    strings::SubstituteAndAppend(contents, "$0, ", range->start);
                } else if (range->end > FieldDescriptor::kMaxNumber) {
                    strings::SubstituteAndAppend(contents, "$0 to max, ", range->start);
                } else {
                    strings::SubstituteAndAppend(contents, "$0 to $1, ", range->start,
                        range->end - 1);
                }
            }
            contents->replace(contents->size() - 2, 2, ";\n");
        }

        if (reserved_name_count() > 0) {
            strings::SubstituteAndAppend(contents, "$0  reserved ", prefix);
            for (int i = 0; i < reserved_name_count(); i++) {
                strings::SubstituteAndAppend(contents, "\"$0\", ",
                    CEscape(reserved_name(i)));
            }
            contents->replace(contents->size() - 2, 2, ";\n");
        }

        strings::SubstituteAndAppend(contents, "$0}\n", prefix);
        comment_printer.AddPostComment(contents);
    }

    bool Descriptor::GetSourceLocation(SourceLocation* out_location) const {
        std::vector<int> path;
        GetLocationPath(&path);
        return file()->GetSourceLocation(path, out_location);
    }

} // namespace shark


#include <shark/port_undef.inc>
