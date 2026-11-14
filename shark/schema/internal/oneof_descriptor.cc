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

#include <shark/schema/internal/oneof_descriptor.h>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    void OneofDescriptor::CopyTo(OneofDescriptorProto* proto) const {
        proto->set_name(name());
        if (&options() != &OneofOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

    std::string OneofDescriptor::DebugString() const {
        DebugStringOptions options; // default values
        return DebugStringWithOptions(options);
    }

    std::string OneofDescriptor::DebugStringWithOptions(
        const DebugStringOptions& options) const {
        std::string contents;
        DebugString(0, &contents, options);
        return contents;
    }

    void OneofDescriptor::DebugString(
        int depth, std::string* contents,
        const DebugStringOptions& debug_string_options) const {
        std::string prefix(depth * 2, ' ');
        ++depth;
        SourceLocationCommentPrinter comment_printer(this, prefix,
            debug_string_options);
        comment_printer.AddPreComment(contents);
        strings::SubstituteAndAppend(contents, "$0oneof $1 {", prefix, name());

        FormatLineOptions(depth, options(), containing_type()->file()->pool(),
            contents);

        if (debug_string_options.elide_oneof_body) {
            contents->append(" ... }\n");
        } else {
            contents->append("\n");
            for (int i = 0; i < field_count(); i++) {
                field(i)->DebugString(depth, contents, debug_string_options);
            }
            strings::SubstituteAndAppend(contents, "$0}\n", prefix);
        }
        comment_printer.AddPostComment(contents);
    }

    bool OneofDescriptor::GetSourceLocation(SourceLocation* out_location) const {
        std::vector<int> path;
        GetLocationPath(&path);
        return containing_type()->file()->GetSourceLocation(path, out_location);
    }

    void OneofDescriptor::GetLocationPath(std::vector<int>* output) const {
        containing_type()->GetLocationPath(output);
        output->push_back(DescriptorProto::kOneofDeclFieldNumber);
        output->push_back(index());
    }

} // namespace shark


#include <shark/port_undef.inc>
