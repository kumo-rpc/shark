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

#include <string>
#include <vector>
#include <shark/schema/debug_string_options.h>
#include <shark/schema/source_location.h>
// Must be included last.
#include <shark/port_def.inc>

namespace shark {

        inline constexpr int kMaxFieldType = static_cast<int> (SharkFieldType::MAX);
    inline constexpr int kMaxFieldCppType = static_cast<int> (SharkFieldCppType::MAX);
    inline constexpr int kMaxFieldLabel = static_cast<int> (FieldLabel::MAX);

    class SHARK_EXPORT FieldDescriptor : private internal::SymbolBase {
    public:
        typedef FieldDescriptorProto Proto;

        // Valid field numbers are positive integers up to kMaxNumber.
        static constexpr int kMaxNumber = (1 << 29) - 1;

        // First field number reserved for the protocol buffer library implementation.
        // Users may not declare fields that use reserved numbers.
        static constexpr int kFirstReservedNumber = 19000;
        // Last field number reserved for the protocol buffer library implementation.
        // Users may not declare fields that use reserved numbers.
        static constexpr int kLastReservedNumber = 19999;

        const std::string& name() const; // Name of this field within the message.
        const std::string& full_name() const; // Fully-qualified name of the field.
        const std::string& json_name() const; // JSON name of this field.
        const FileDescriptor* file() const; // File in which this field was defined.
        bool is_extension() const; // Is this an extension field?
        int number() const; // Declared tag number.

        // Same as name() except converted to lower-case.  This (and especially the
        // FindFieldByLowercaseName() method) can be useful when parsing formats
        // which prefer to use lowercase naming style.  (Although, technically
        // field names should be lowercased anyway according to the protobuf style
        // guide, so this only makes a difference when dealing with old .shark files
        // which do not follow the guide.)
        const std::string& lowercase_name() const;

        // Same as name() except converted to camel-case.  In this conversion, any
        // time an underscore appears in the name, it is removed and the next
        // letter is capitalized.  Furthermore, the first letter of the name is
        // lower-cased.  Examples:
        //   FooBar -> fooBar
        //   foo_bar -> fooBar
        //   fooBar -> fooBar
        // This (and especially the FindFieldByCamelcaseName() method) can be useful
        // when parsing formats which prefer to use camel-case naming style.
        const std::string& camelcase_name() const;

        SharkFieldType type() const; // Declared type of this field.
        const char* type_name() const; // Name of the declared type.
        SharkFieldCppType cpp_type() const; // C++ type of this field.
        const char* cpp_type_name() const; // Name of the C++ type.
        FieldLabel label() const; // optional/required/repeated

        bool is_required() const; // shorthand for label() == FieldLabel::REQUIRED
        bool is_optional() const; // shorthand for label() == FieldLabel::OPTIONAL
        bool is_repeated() const; // shorthand for label() == FieldLabel::REPEATED
        bool is_packable() const; // shorthand for is_repeated() &&
                                  //               IsTypePackable(type())
        bool is_packed() const; // shorthand for is_packable() &&
                                //               options().packed()
        bool is_map() const; // shorthand for type() == SharkFieldType::MESSAGE &&
                             // message_type()->options().map_entry()

        // Returns true if this field was syntactically written with "optional" in the
        // .shark file. Excludes singular proto3 fields that do not have a label.
        bool has_optional_keyword() const;

        // Returns true if this field tracks presence, ie. does the field
        // distinguish between "unset" and "present with default value."
        // This includes required, optional, and oneof fields. It excludes maps,
        // repeated fields, and singular proto3 fields without "optional".
        //
        // For fields where has_presence() == true, the return value of
        // Reflection::HasField() is semantically meaningful.
        bool has_presence() const;

        // Index of this field within the message's field array, or the file or
        // extension scope's extensions array.
        int index() const;

        // Does this field have an explicitly-declared default value?
        bool has_default_value() const;

        // Whether the user has specified the json_name field option in the .shark
        // file.
        bool has_json_name() const;

        // Get the field default value if cpp_type() == SharkFieldCppType::INT32.  If no
        // explicit default was defined, the default is 0.
        int32_t default_value_int32_t() const;
        int32_t default_value_int32() const { return default_value_int32_t(); }
        // Get the field default value if cpp_type() == SharkFieldCppType::INT64.  If no
        // explicit default was defined, the default is 0.
        int64_t default_value_int64_t() const;
        int64_t default_value_int64() const { return default_value_int64_t(); }
        // Get the field default value if cpp_type() == SharkFieldCppType::UINT32.  If no
        // explicit default was defined, the default is 0.
        uint32_t default_value_uint32_t() const;
        uint32_t default_value_uint32() const { return default_value_uint32_t(); }
        // Get the field default value if cpp_type() == SharkFieldCppType::UINT64.  If no
        // explicit default was defined, the default is 0.
        uint64_t default_value_uint64_t() const;
        uint64_t default_value_uint64() const { return default_value_uint64_t(); }
        // Get the field default value if cpp_type() == SharkFieldCppType::FLOAT.  If no
        // explicit default was defined, the default is 0.0.
        float default_value_float() const;
        // Get the field default value if cpp_type() == SharkFieldCppType::DOUBLE.  If no
        // explicit default was defined, the default is 0.0.
        double default_value_double() const;
        // Get the field default value if cpp_type() == SharkFieldCppType::BOOL.  If no
        // explicit default was defined, the default is false.
        bool default_value_bool() const;
        // Get the field default value if cpp_type() == SharkFieldCppType::ENUM.  If no
        // explicit default was defined, the default is the first value defined
        // in the enum type (all enum types are required to have at least one value).
        // This never returns nullptr.
        const EnumValueDescriptor* default_value_enum() const;
        // Get the field default value if cpp_type() == SharkFieldCppType::STRING.  If no
        // explicit default was defined, the default is the empty string.
        const std::string& default_value_string() const;

        // The Descriptor for the message of which this is a field.  For extensions,
        // this is the extended type.  Never nullptr.
        const Descriptor* containing_type() const;

        // If the field is a member of a oneof, this is the one, otherwise this is
        // nullptr.
        const OneofDescriptor* containing_oneof() const;

        // If the field is a member of a non-synthetic oneof, returns the descriptor
        // for the oneof, otherwise returns nullptr.
        const OneofDescriptor* real_containing_oneof() const;

        // If the field is a member of a oneof, returns the index in that oneof.
        int index_in_oneof() const;

        // An extension may be declared within the scope of another message.  If this
        // field is an extension (is_extension() is true), then extension_scope()
        // returns that message, or nullptr if the extension was declared at global
        // scope.  If this is not an extension, extension_scope() is undefined (may
        // assert-fail).
        const Descriptor* extension_scope() const;

        // If type is SharkFieldType::MESSAGE or SharkFieldType::GROUP, returns a descriptor for the
        // message or the group type.  Otherwise, returns null.
        const Descriptor* message_type() const;
        // If type is SharkFieldType::ENUM, returns a descriptor for the enum.  Otherwise,
        // returns null.
        const EnumDescriptor* enum_type() const;

        // Get the FieldOptions for this field.  This includes things listed in
        // square brackets after the field definition.  E.g., the field:
        //   optional string text = 1 [ctype=CORD];
        // has the "ctype" option set.  Allowed options are defined by FieldOptions in
        // descriptor.shark, and any available extensions of that message.
        const FieldOptions& options() const;

        // See Descriptor::CopyTo().
        void CopyTo(FieldDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        static SharkFieldCppType TypeToCppType(SharkFieldType type);

        static const char* TypeName(SharkFieldType type);

        static const char* CppTypeName(SharkFieldCppType cpp_type);

        // Return true iff [packed = true] is valid for fields of this type.
        static inline bool IsTypePackable(SharkFieldType field_type);

        // Returns full_name() except if the field is a MessageSet extension,
        // in which case it returns the full_name() of the containing message type
        // for backwards compatibility with proto1.
        //
        // A MessageSet extension is defined as an optional message extension
        // whose containing type has the message_set_wire_format option set.
        // This should be true of extensions of shark.bridge.MessageSet;
        // by convention, such extensions are named "message_set_extension".
        //
        // The opposite operation (looking up an extension's FieldDescriptor given
        // its printable name) can be accomplished with
        //     message->file()->pool()->FindExtensionByPrintableName(message, name)
        // where the extension extends "message".
        const std::string& PrintableNameForExtension() const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this field declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef FieldOptions OptionsType;

        // Allows access to GetLocationPath for annotations.
        friend class io::Printer;
        friend class compiler::cpp::Formatter;
        friend class compiler::cc::Formatter;
        friend class Reflection;

        // Fill the json_name field of FieldDescriptorProto.
        void CopyJsonNameTo(FieldDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        void DebugString(int depth, std::string* contents,
            const DebugStringOptions& options) const;

        // formats the default value appropriately and returns it as a string.
        // Must have a default value to call this. If quote_string_type is true, then
        // types of SharkFieldCppType::STRING will be surrounded by quotes and CEscaped.
        std::string DefaultValueAsString(bool quote_string_type) const;

        // Helper function that returns the field type name for DebugString.
        std::string FieldTypeNameDebugString() const;

        // Walks up the descriptor tree to generate the source location path
        // to this descriptor from the file root.
        void GetLocationPath(std::vector<int>* output) const;

        // Returns true if this is a map message type.
        bool is_map_message_type() const;

        bool has_default_value_ : 1;
        bool proto3_optional_ : 1;
        // Whether the user has specified the json_name field option in the .shark
        // file.
        bool has_json_name_ : 1;
        bool is_extension_ : 1;
        bool is_oneof_ : 1;

        // Actually a `Label` but stored as uint8_t to save space.
        uint8_t label_ ;

        // Actually a `Type`, but stored as uint8_t to save space.
        mutable uint8_t type_;

        // Logically:
        //   all_names_ = [name, full_name, lower, camel, json]
        // However:
        //   duplicates will be omitted, so lower/camel/json might be in the same
        //   position.
        // We store the true offset for each name here, and the bit width must be
        // large enough to account for the worst case where all names are present.
        uint8_t lowercase_name_index_ : 2;
        uint8_t camelcase_name_index_ : 2;
        uint8_t json_name_index_ : 3;
        // Sadly, `number_` located here to reduce padding. Unrelated to all_names_
        // and its indices above.
        int number_;
        const std::string* all_names_;
        const FileDescriptor* file_;

        // The once_flag is followed by a NUL terminated string for the type name and
        // enum default value (or empty string if no default enum).
        internal::once_flag* type_once_;
        static void TypeOnceInit(const FieldDescriptor* to_init);
        void InternalTypeOnceInit() const;
        const Descriptor* containing_type_;
        union {
            const OneofDescriptor* containing_oneof;
            const Descriptor* extension_scope;
        } scope_;
        union {
            mutable const Descriptor* message_type;
            mutable const EnumDescriptor* enum_type;
        } type_descriptor_;
        const FieldOptions* options_;
        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<FieldDescriptor>() and AllocateArray<FieldDescriptor>() in
        // descriptor.cc and update them to initialize the field.

        union {
            int32_t default_value_int32_t_;
            int64_t default_value_int64_t_;
            uint32_t default_value_uint32_t_;
            uint64_t default_value_uint64_t_;
            float default_value_float_;
            double default_value_double_;
            bool default_value_bool_;

            mutable const EnumValueDescriptor* default_value_enum_;
            const std::string* default_value_string_;
            mutable std::atomic<const Message*> default_generated_instance_;
        };

        static const SharkFieldCppType kTypeToCppTypeMap[kMaxFieldType + 1];

        static const char* const kTypeToName[kMaxFieldType + 1];

        static const char* const kCppTypeToName[kMaxFieldCppType + 1];

        static const char* const kLabelToName[kMaxFieldLabel + 1];

        // Must be constructed using DescriptorPool.
        FieldDescriptor() { }
        friend class DescriptorBuilder;
        friend class FileDescriptor;
        friend class Descriptor;
        friend class OneofDescriptor;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldDescriptor);
    };


    SHARK_INTERNAL_CHECK_CLASS_SIZE(FieldDescriptor, 72);

}  // namespace shark

#include <shark/port_undef.inc>
