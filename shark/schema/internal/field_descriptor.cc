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

#include <shark/schema/internal/utils.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>
// Must be included last.
#include <shark/port_def.inc>

namespace shark {

        const SharkFieldCppType
        FieldDescriptor::kTypeToCppTypeMap[kMaxFieldType + 1] = {
            static_cast<SharkFieldCppType>(0), // 0 is reserved for errors

            SharkFieldCppType::CPPTYPE_BOOL, // SharkFieldType::BOOL
            SharkFieldCppType::CPPTYPE_INT8, // SharkFieldType::INT8
            SharkFieldCppType::CPPTYPE_UINT8, // SharkFieldType::UINT8
            SharkFieldCppType::CPPTYPE_INT16, // SharkFieldType::INT16
            SharkFieldCppType::CPPTYPE_UINT16, // SharkFieldType::UINT16
            SharkFieldCppType::CPPTYPE_INT32, // SharkFieldType::INT32
            SharkFieldCppType::CPPTYPE_UINT32, // SharkFieldType::UINT32
            SharkFieldCppType::CPPTYPE_INT64, // SharkFieldType::INT64
            SharkFieldCppType::CPPTYPE_UINT64, // SharkFieldType::UINT64
            SharkFieldCppType::CPPTYPE_FLOAT, // SharkFieldType::FLOAT
            SharkFieldCppType::CPPTYPE_FLOAT16, // SharkFieldType::FLOAT16
            SharkFieldCppType::CPPTYPE_FLOAT8, // SharkFieldType::FLOAT8
            SharkFieldCppType::CPPTYPE_DOUBLE, // SharkFieldType::DOUBLE
            SharkFieldCppType::CPPTYPE_ENUM, // SharkFieldType::ENUM
            SharkFieldCppType::CPPTYPE_MESSAGE, // SharkFieldType::MESSAGE
            SharkFieldCppType::CPPTYPE_STRING, // SharkFieldType::STRING
            SharkFieldCppType::CPPTYPE_STRING, // SharkFieldType::BYTES
            SharkFieldCppType::CPPTYPE_UINT32, // SharkFieldType::FIXED32
            SharkFieldCppType::CPPTYPE_UINT64, // SharkFieldType::FIXED64
            SharkFieldCppType::CPPTYPE_INT32, // SharkFieldType::SFIXED32
            SharkFieldCppType::CPPTYPE_INT64, // SharkFieldType::SFIXED64
            SharkFieldCppType::CPPTYPE_INT32, // SharkFieldType::SINT32
            SharkFieldCppType::CPPTYPE_INT64, // SharkFieldType::SINT64
        };

    const char* const FieldDescriptor::kTypeToName[kMaxFieldType + 1] = {
        "ERROR", // 0 is reserved for errors

        "bool", // SharkFieldType::BOOL
        "int8", // SharkFieldType::INT8
        "uint8", // SharkFieldType::UINT8
        "int16", // SharkFieldType::INT16
        "uint16", // SharkFieldType::UINT16
        "int32", // SharkFieldType::INT32
        "uint32", // SharkFieldType::UINT32
        "int64", // SharkFieldType::INT64
        "uint64", // SharkFieldType::UINT64
        "float", // SharkFieldType::FLOAT
        "float16", // SharkFieldType::FLOAT16
        "float8", // SharkFieldType::FLOAT8
        "double", // SharkFieldType::DOUBLE
        "enum", // SharkFieldType::ENUM
        "message", // SharkFieldType::MESSAGE
        "string", // SharkFieldType::STRING
        "bytes", // SharkFieldType::BYTES
        "fixed32", // SharkFieldType::FIXED32
        "fixed64", // SharkFieldType::FIXED64
        "sfixed32", // SharkFieldType::SFIXED32
        "sfixed64", // SharkFieldType::SFIXED64
        "sint32", // SharkFieldType::SINT32
        "sint64", // SharkFieldType::SINT64
    };

    const char* const FieldDescriptor::kCppTypeToName[kMaxFieldCppType + 1] = {
        "ERROR", // 0 is reserved for errors

        "bool", // SharkFieldCppType::CPPTYPE_BOOL
        "int8", // SharkFieldCppType::CPPTYPE_INT8
        "uint8", // SharkFieldCppType::CPPTYPE_UINT8
        "int16", // SharkFieldCppType::CPPTYPE_INT16
        "uint16", // SharkFieldCppType::CPPTYPE_UINT16
        "int32", // SharkFieldCppType::CPPTYPE_INT32
        "uint32", // SharkFieldCppType::CPPTYPE_UINT32
        "int64", // SharkFieldCppType::CPPTYPE_INT64
        "uint64", // SharkFieldCppType::CPPTYPE_UINT64
        "float", // SharkFieldCppType::CPPTYPE_FLOAT
        "float16", // SharkFieldCppType::CPPTYPE_FLOAT16
        "float8", // SharkFieldCppType::CPPTYPE_FLOAT8
        "double", // SharkFieldCppType::CPPTYPE_DOUBLE
        "enum", // SharkFieldCppType::CPPTYPE_ENUM
        "message", // SharkFieldCppType::CPPTYPE_MESSAGE
        "string", // SharkFieldCppType::CPPTYPE_STRING
    };

    const char* const FieldDescriptor::kLabelToName[kMaxFieldLabel + 1] = {
        "ERROR", // 0 is reserved for errors

        "optional", // FieldLabel::OPTIONAL
        "required", // FieldLabel::REQUIRED
        "repeated", // FieldLabel::REPEATED
    };

     void FieldDescriptor::CopyTo(FieldDescriptorProto* proto) const {
        proto->set_name(name());
        proto->set_number(number());
        if (has_json_name_) {
            proto->set_json_name(json_name());
        }
        if (proto3_optional_) {
            proto->set_proto3_optional(true);
        }
        // Some compilers do not allow static_cast directly between two enum types,
        // so we must cast to int first.
        proto->set_label(static_cast<FieldCardinality>(
            static_cast<int>(label())));
        proto->set_type(static_cast<SharkType>(
            static_cast<int>(type())));

        if (is_extension()) {
            if (!containing_type()->is_unqualified_placeholder_) {
                proto->set_extendee(".");
            }
            proto->mutable_extendee()->append(containing_type()->full_name());
        }

        if (cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE) {
            if (message_type()->is_placeholder_) {
                // We don't actually know if the type is a message type.  It could be
                // an enum.
                proto->clear_type();
            }

            if (!message_type()->is_unqualified_placeholder_) {
                proto->set_type_name(".");
            }
            proto->mutable_type_name()->append(message_type()->full_name());
        } else if (cpp_type() == SharkFieldCppType::CPPTYPE_ENUM) {
            if (!enum_type()->is_unqualified_placeholder_) {
                proto->set_type_name(".");
            }
            proto->mutable_type_name()->append(enum_type()->full_name());
        }

        if (has_default_value()) {
            proto->set_default_value(DefaultValueAsString(false));
        }

        if (containing_oneof() != nullptr && !is_extension()) {
            proto->set_oneof_index(containing_oneof()->index());
        }

        if (&options() != &FieldOptions::default_instance()) {
            proto->mutable_options()->CopyFrom(options());
        }
    }

    void FieldDescriptor::CopyJsonNameTo(FieldDescriptorProto* proto) const {
        proto->set_json_name(json_name());
    }

    std::string FieldDescriptor::DebugString() const {
        DebugStringOptions options; // default options
        return DebugStringWithOptions(options);
    }

    std::string FieldDescriptor::DebugStringWithOptions(
        const DebugStringOptions& debug_string_options) const {
        std::string contents;
        int depth = 0;
        if (is_extension()) {
            strings::SubstituteAndAppend(&contents, "extend .$0 {\n",
                containing_type()->full_name());
            depth = 1;
        }
        DebugString(depth, &contents, debug_string_options);
        if (is_extension()) {
            contents.append("}\n");
        }
        return contents;
    }

    // The field type string used in FieldDescriptor::DebugString()
    std::string FieldDescriptor::FieldTypeNameDebugString() const {
        switch (type()) {
        case SharkFieldType::MESSAGE:
            return "." + message_type()->full_name();
        case SharkFieldType::ENUM:
            return "." + enum_type()->full_name();
        default:
            return kTypeToName[static_cast<int>(type())];
        }
    }

    void FieldDescriptor::DebugString(
        int depth, std::string* contents,
        const DebugStringOptions& debug_string_options) const {
        std::string prefix(depth * 2, ' ');
        std::string field_type;

        // Special case map fields.
        if (is_map()) {
            strings::SubstituteAndAppend(
                &field_type, "map<$0, $1>",
                message_type()->field(0)->FieldTypeNameDebugString(),
                message_type()->field(1)->FieldTypeNameDebugString());
        } else if (is_repeated()) {
            field_type = "list<" + FieldTypeNameDebugString() + ">";
        } else {
            field_type = FieldTypeNameDebugString();
        }

        std::string label = StrCat(kLabelToName[static_cast<int>(this->label())], " ");

        // Label is omitted for maps, oneof, and plain proto3 fields.
        if (is_map() || is_repeated() || real_containing_oneof() ||
            (is_optional() && !has_optional_keyword())) {
            label.clear();
        }

        SourceLocationCommentPrinter comment_printer(this, prefix,
            debug_string_options);
        comment_printer.AddPreComment(contents);

        strings::SubstituteAndAppend(
            contents, "$0$1$2 $3 = $4", prefix, label, field_type, name(),
            number());

        bool bracketed = false;
        if (has_default_value()) {
            bracketed = true;
            strings::SubstituteAndAppend(contents, " [default = $0",
                DefaultValueAsString(true));
        }
        if (has_json_name_) {
            if (!bracketed) {
                bracketed = true;
                contents->append(" [");
            } else {
                contents->append(", ");
            }
            contents->append("json_name = \"");
            contents->append(CEscape(json_name()));
            contents->append("\"");
        }

        std::string formatted_options;
        if (FormatBracketedOptions(depth, options(), file()->pool(),
                &formatted_options)) {
            contents->append(bracketed ? ", " : " [");
            bracketed = true;
            contents->append(formatted_options);
        }

        if (bracketed) {
            contents->append("]");
        }

        contents->append(";\n");

        comment_printer.AddPostComment(contents);
    }

    bool FieldDescriptor::GetSourceLocation(SourceLocation* out_location) const {
         std::vector<int> path;
         GetLocationPath(&path);
         return file()->GetSourceLocation(path, out_location);
     }

    void FieldDescriptor::GetLocationPath(std::vector<int>* output) const {
         if (is_extension()) {
             if (extension_scope() == nullptr) {
                 output->push_back(FileDescriptorProto::kExtensionFieldNumber);
                 output->push_back(index());
             } else {
                 extension_scope()->GetLocationPath(output);
                 output->push_back(DescriptorProto::kExtensionFieldNumber);
                 output->push_back(index());
             }
         } else {
             containing_type()->GetLocationPath(output);
             output->push_back(DescriptorProto::kFieldFieldNumber);
             output->push_back(index());
         }
     }


} // namespace shark


#include <shark/port_undef.inc>
