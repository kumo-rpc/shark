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
#include <shark/schema/internal/enum_descriptor.h>
#include <shark/schema/internal/field_descriptor.h>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    const EnumValueDescriptor* EnumDescriptor::FindValueByName(
        ConstStringParam key) const {
        return file()->tables_->FindNestedSymbol(this, key).enum_value_descriptor();
    }

    const EnumValueDescriptor* EnumDescriptor::FindValueByNumber(int key) const {
        return file()->tables_->FindEnumValueByNumber(this, key);
    }

    const EnumValueDescriptor* EnumDescriptor::FindValueByNumberCreatingIfUnknown(
        int key) const {
        return file()->tables_->FindEnumValueByNumberCreatingIfUnknown(this, key);
    }

    void EnumDescriptor::CopyTo(EnumDescriptorProto* proto) const {
        proto->set_name(name());

        for (int i = 0; i < value_count(); i++) {
            value(i)->CopyTo(proto->add_value());
        }
        for (int i = 0; i < reserved_range_count(); i++) {
            EnumDescriptorProto::EnumReservedRange* range = proto->add_reserved_range();
            range->set_start(reserved_range(i)->start);
            range->set_end(reserved_range(i)->end);
        }
        for (int i = 0; i < reserved_name_count(); i++) {
            proto->add_reserved_name(reserved_name(i));
        }

        if (&options() != &EnumOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

    void EnumValueDescriptor::CopyTo(EnumValueDescriptorProto* proto) const {
        proto->set_name(name());
        proto->set_number(number());

        if (&options() != &EnumValueOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

     std::string EnumDescriptor::DebugString() const {
        DebugStringOptions options; // default values
        return DebugStringWithOptions(options);
    }

    std::string EnumDescriptor::DebugStringWithOptions(
        const DebugStringOptions& options) const {
        std::string contents;
        DebugString(0, &contents, options);
        return contents;
    }

    void EnumDescriptor::DebugString(
        int depth, std::string* contents,
        const DebugStringOptions& debug_string_options) const {
        std::string prefix(depth * 2, ' ');
        ++depth;

        SourceLocationCommentPrinter comment_printer(this, prefix,
            debug_string_options);
        comment_printer.AddPreComment(contents);

        strings::SubstituteAndAppend(contents, "$0enum $1 {\n", prefix, name());

        FormatLineOptions(depth, options(), file()->pool(), contents);

        for (int i = 0; i < value_count(); i++) {
            value(i)->DebugString(depth, contents, debug_string_options);
        }

        if (reserved_range_count() > 0) {
            strings::SubstituteAndAppend(contents, "$0  reserved ", prefix);
            for (int i = 0; i < reserved_range_count(); i++) {
                const EnumDescriptor::ReservedRange* range = reserved_range(i);
                if (range->end == range->start) {
                    strings::SubstituteAndAppend(contents, "$0, ", range->start);
                } else if (range->end == INT_MAX) {
                    strings::SubstituteAndAppend(contents, "$0 to max, ", range->start);
                } else {
                    strings::SubstituteAndAppend(contents, "$0 to $1, ", range->start,
                        range->end);
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

    std::string EnumValueDescriptor::DebugString() const {
        DebugStringOptions options; // default values
        return DebugStringWithOptions(options);
    }

    std::string EnumValueDescriptor::DebugStringWithOptions(
        const DebugStringOptions& options) const {
        std::string contents;
        DebugString(0, &contents, options);
        return contents;
    }

    void EnumValueDescriptor::DebugString(
        int depth, std::string* contents,
        const DebugStringOptions& debug_string_options) const {
        std::string prefix(depth * 2, ' ');

        SourceLocationCommentPrinter comment_printer(this, prefix,
            debug_string_options);
        comment_printer.AddPreComment(contents);

        strings::SubstituteAndAppend(contents, "$0$1 = $2", prefix, name(), number());

        std::string formatted_options;
        if (FormatBracketedOptions(depth, options(), type()->file()->pool(),
                &formatted_options)) {
            strings::SubstituteAndAppend(contents, " [$0]", formatted_options);
        }
        contents->append(";\n");

        comment_printer.AddPostComment(contents);
    }

    bool EnumDescriptor::GetSourceLocation(SourceLocation* out_location) const {
        std::vector<int> path;
        GetLocationPath(&path);
        return file()->GetSourceLocation(path, out_location);
    }

    bool EnumValueDescriptor::GetSourceLocation(
        SourceLocation* out_location) const {
        std::vector<int> path;
        GetLocationPath(&path);
        return type()->file()->GetSourceLocation(path, out_location);
    }


    void EnumDescriptor::GetLocationPath(std::vector<int>* output) const {
        if (containing_type()) {
            containing_type()->GetLocationPath(output);
            output->push_back(DescriptorProto::kEnumTypeFieldNumber);
            output->push_back(index());
        } else {
            output->push_back(FileDescriptorProto::kEnumTypeFieldNumber);
            output->push_back(index());
        }
    }

    void EnumValueDescriptor::GetLocationPath(std::vector<int>* output) const {
        type()->GetLocationPath(output);
        output->push_back(EnumDescriptorProto::kValueFieldNumber);
        output->push_back(index());
    }

} // namespace shark


#include <shark/port_undef.inc>
