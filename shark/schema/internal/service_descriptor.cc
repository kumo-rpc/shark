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

#include <shark/schema/internal/service_descriptor.h>
#include <shark/schema/internal/file_descriptor_tables.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>
// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    const MethodDescriptor* ServiceDescriptor::FindMethodByName(
       ConstStringParam key) const {
        return file()->tables_->FindNestedSymbol(this, key).method_descriptor();
    }


    void ServiceDescriptor::CopyTo(ServiceDescriptorProto* proto) const {
        proto->set_name(name());

        for (int i = 0; i < method_count(); i++) {
            method(i)->CopyTo(proto->add_method());
        }

        if (&options() != &ServiceOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

    void MethodDescriptor::CopyTo(MethodDescriptorProto* proto) const {
        proto->set_name(name());

        if (!input_type()->is_unqualified_placeholder_) {
            proto->set_input_type(".");
        }
        proto->mutable_input_type()->append(input_type()->full_name());

        if (!output_type()->is_unqualified_placeholder_) {
            proto->set_output_type(".");
        }
        proto->mutable_output_type()->append(output_type()->full_name());

        if (&options() != &MethodOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }

        if (client_streaming_) {
            proto->set_client_streaming(true);
        }
        if (server_streaming_) {
            proto->set_server_streaming(true);
        }
    }

     std::string ServiceDescriptor::DebugString() const {
        DebugStringOptions options; // default values
        return DebugStringWithOptions(options);
    }

    std::string ServiceDescriptor::DebugStringWithOptions(
        const DebugStringOptions& options) const {
        std::string contents;
        DebugString(&contents, options);
        return contents;
    }

    void ServiceDescriptor::DebugString(
        std::string* contents,
        const DebugStringOptions& debug_string_options) const {
        SourceLocationCommentPrinter comment_printer(this, /* prefix */ "",
            debug_string_options);
        comment_printer.AddPreComment(contents);

        strings::SubstituteAndAppend(contents, "service $0 {\n", name());

        FormatLineOptions(1, options(), file()->pool(), contents);

        for (int i = 0; i < method_count(); i++) {
            method(i)->DebugString(1, contents, debug_string_options);
        }

        contents->append("}\n");

        comment_printer.AddPostComment(contents);
    }

    std::string MethodDescriptor::DebugString() const {
        DebugStringOptions options; // default values
        return DebugStringWithOptions(options);
    }

    std::string MethodDescriptor::DebugStringWithOptions(
        const DebugStringOptions& options) const {
        std::string contents;
        DebugString(0, &contents, options);
        return contents;
    }

    void MethodDescriptor::DebugString(
        int depth, std::string* contents,
        const DebugStringOptions& debug_string_options) const {
        std::string prefix(depth * 2, ' ');
        ++depth;

        SourceLocationCommentPrinter comment_printer(this, prefix,
            debug_string_options);
        comment_printer.AddPreComment(contents);

        strings::SubstituteAndAppend(
            contents, "$0rpc $1($4.$2) returns ($5.$3)", prefix, name(),
            input_type()->full_name(), output_type()->full_name(),
            client_streaming() ? "stream " : "", server_streaming() ? "stream " : "");

        std::string formatted_options;
        if (FormatLineOptions(depth, options(), service()->file()->pool(),
                &formatted_options)) {
            strings::SubstituteAndAppend(contents, " {\n$0$1}\n", formatted_options,
                prefix);
        } else {
            contents->append(";\n");
        }

        comment_printer.AddPostComment(contents);
    }

    bool MethodDescriptor::GetSourceLocation(SourceLocation* out_location) const {
        std::vector<int> path;
        GetLocationPath(&path);
        return service()->file()->GetSourceLocation(path, out_location);
    }

    bool ServiceDescriptor::GetSourceLocation(SourceLocation* out_location) const {
        std::vector<int> path;
        GetLocationPath(&path);
        return file()->GetSourceLocation(path, out_location);
    }



    void ServiceDescriptor::GetLocationPath(std::vector<int>* output) const {
        output->push_back(FileDescriptorProto::kServiceFieldNumber);
        output->push_back(index());
    }

    void MethodDescriptor::GetLocationPath(std::vector<int>* output) const {
        service()->GetLocationPath(output);
        output->push_back(ServiceDescriptorProto::kMethodFieldNumber);
        output->push_back(index());
    }


    const Descriptor* MethodDescriptor::input_type() const {
        return input_type_.Get(service());
    }

    const Descriptor* MethodDescriptor::output_type() const {
        return output_type_.Get(service());
    }


} // namespace shark


#include <shark/port_undef.inc>
