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

#include <shark/schema/fwd.h>
#include <shark/schema/field_descriptor.h>
#include <shark/schema/enum_descriptor.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

        // Describes a type of protocol message, or a particular group within a
    // message.  To obtain the Descriptor for a given message object, call
    // Message::GetDescriptor().  Generated message classes also have a
    // static method called descriptor() which returns the type's descriptor.
    // Use DescriptorPool to construct your own descriptors.
    class SHARK_EXPORT Descriptor : private internal::SymbolBase {
    public:
        typedef DescriptorProto Proto;

        // The name of the message type, not including its scope.
        [[nodiscard]] const std::string& name() const;

        // The fully-qualified name of the message type, scope delimited by
        // periods.  For example, message type "Foo" which is declared in package
        // "bar" has full name "bar.Foo".  If a type "Baz" is nested within
        // Foo, Baz's full_name is "bar.Foo.Baz".  To get only the part that
        // comes after the last '.', use name().
        [[nodiscard]] const std::string& full_name() const;

        // Index of this descriptor within the file or containing type's message
        // type array.
        [[nodiscard]] int index() const;

        // The .shark file in which this message type was defined.  Never nullptr.
        [[nodiscard]] const FileDescriptor* file() const;

        // If this Descriptor describes a nested type, this returns the type
        // in which it is nested.  Otherwise, returns nullptr.
        [[nodiscard]] const Descriptor* containing_type() const;

        // Get options for this message type.  These are specified in the .shark file
        // by placing lines like "option foo = 1234;" in the message definition.
        // Allowed options are defined by MessageOptions in descriptor.shark, and any
        // available extensions of that message.
        [[nodiscard]] const MessageOptions& options() const;

        // Write the contents of this Descriptor into the given DescriptorProto.
        // The target DescriptorProto must be clear before calling this; if it
        // isn't, the result may be garbage.
        void CopyTo(DescriptorProto* proto) const;

        // Write the contents of this descriptor in a human-readable form. Output
        // will be suitable for re-parsing.
        [[nodiscard]] std::string DebugString() const;

        // Similar to DebugString(), but additionally takes options (e.g.,
        // include original user comments in output).
        [[nodiscard]] std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Returns true if this is a placeholder for an unknown type. This will
        // only be the case if this descriptor comes from a DescriptorPool
        // with AllowUnknownDependencies() set.
        [[nodiscard]] bool is_placeholder() const;

        enum WellKnownType {
            WELLKNOWNTYPE_UNSPECIFIED, // Not a well-known type.

            // Wrapper types.
            WELLKNOWNTYPE_DOUBLEVALUE, // shark.DoubleValue
            WELLKNOWNTYPE_FLOATVALUE, // shark.FloatValue
            WELLKNOWNTYPE_INT64VALUE, // shark.Int64Value
            WELLKNOWNTYPE_UINT64VALUE, // shark.UInt64Value
            WELLKNOWNTYPE_INT32VALUE, // shark.Int32Value
            WELLKNOWNTYPE_UINT32VALUE, // shark.UInt32Value
            WELLKNOWNTYPE_STRINGVALUE, // shark.StringValue
            WELLKNOWNTYPE_BYTESVALUE, // shark.BytesValue
            WELLKNOWNTYPE_BOOLVALUE, // shark.BoolValue

            // Other well known types.
            WELLKNOWNTYPE_ANY, // shark.Any
            WELLKNOWNTYPE_FIELDMASK, // shark.FieldMask
            WELLKNOWNTYPE_DURATION, // shark.Duration
            WELLKNOWNTYPE_TIMESTAMP, // shark.Timestamp
            WELLKNOWNTYPE_VALUE, // shark.Value
            WELLKNOWNTYPE_LISTVALUE, // shark.ListValue
            WELLKNOWNTYPE_STRUCT, // shark.Struct

            // New well-known types may be added in the future.
            // Please make sure any switch() statements have a 'default' case.
            __WELLKNOWNTYPE__DO_NOT_USE__ADD_DEFAULT_INSTEAD__,
        };

       [[nodiscard]]  WellKnownType well_known_type() const;

        // Field stuff -----------------------------------------------------

        // The number of fields in this message type.
        [[nodiscard]] int field_count() const;
        // Gets a field by index, where 0 <= index < field_count().
        // These are returned in the order they were defined in the .shark file.
       [[nodiscard]]  const FieldDescriptor* field(int index) const;

        // Looks up a field by declared tag number.  Returns nullptr if no such field
        // exists.
        [[nodiscard]] const FieldDescriptor* FindFieldByNumber(int number) const;
        // Looks up a field by name.  Returns nullptr if no such field exists.
        [[nodiscard]] const FieldDescriptor* FindFieldByName(ConstStringParam name) const;

        // Looks up a field by lowercased name (as returned by lowercase_name()).
        // This lookup may be ambiguous if multiple field names differ only by case,
        // in which case the field returned is chosen arbitrarily from the matches.
        [[nodiscard]] const FieldDescriptor* FindFieldByLowercaseName(
            ConstStringParam lowercase_name) const;

        // Looks up a field by camel-case name (as returned by camelcase_name()).
        // This lookup may be ambiguous if multiple field names differ in a way that
        // leads them to have identical camel-case names, in which case the field
        // returned is chosen arbitrarily from the matches.
        [[nodiscard]] const FieldDescriptor* FindFieldByCamelcaseName(
            ConstStringParam camelcase_name) const;

        // The number of oneofs in this message type.
        [[nodiscard]] int oneof_decl_count() const;
        // The number of oneofs in this message type, excluding synthetic oneofs.
        // Real oneofs always come first, so iterating up to real_oneof_decl_cout()
        // will yield all real oneofs.
        [[nodiscard]] int real_oneof_decl_count() const;
        // Get a oneof by index, where 0 <= index < oneof_decl_count().
        // These are returned in the order they were defined in the .shark file.
        [[nodiscard]] const OneofDescriptor* oneof_decl(int index) const;

        // Looks up a oneof by name.  Returns nullptr if no such oneof exists.
        [[nodiscard]] const OneofDescriptor* FindOneofByName(ConstStringParam name) const;

        // Nested type stuff -----------------------------------------------

        // The number of nested types in this message type.
        [[nodiscard]] int nested_type_count() const;
        // Gets a nested type by index, where 0 <= index < nested_type_count().
        // These are returned in the order they were defined in the .shark file.
        [[nodiscard]] const Descriptor* nested_type(int index) const;

        // Looks up a nested type by name.  Returns nullptr if no such nested type
        // exists.
        [[nodiscard]] const Descriptor* FindNestedTypeByName(ConstStringParam name) const;

        // Enum stuff ------------------------------------------------------

        // The number of enum types in this message type.
       [[nodiscard]]  int enum_type_count() const;
        // Gets an enum type by index, where 0 <= index < enum_type_count().
        // These are returned in the order they were defined in the .shark file.
        [[nodiscard]] const EnumDescriptor* enum_type(int index) const;

        // Looks up an enum type by name.  Returns nullptr if no such enum type
        // exists.
        [[nodiscard]] const EnumDescriptor* FindEnumTypeByName(ConstStringParam name) const;

        // Looks up an enum value by name, among all enum types in this message.
        // Returns nullptr if no such value exists.
        [[nodiscard]] const EnumValueDescriptor* FindEnumValueByName(ConstStringParam name) const;

        // Extensions ------------------------------------------------------

        // A range of field numbers which are designated for third-party
        // extensions.
        struct ExtensionRange {
            typedef DescriptorProto_ExtensionRange Proto;

            typedef ExtensionRangeOptions OptionsType;

            // See Descriptor::CopyTo().
            void CopyTo(DescriptorProto_ExtensionRange* proto) const;

            int start; // inclusive
            int end; // exclusive

            const ExtensionRangeOptions* options_;
        };

        // The number of extension ranges in this message type.
        [[nodiscard]] int extension_range_count() const;
        // Gets an extension range by index, where 0 <= index <
        // extension_range_count(). These are returned in the order they were defined
        // in the .shark file.
        [[nodiscard]] const ExtensionRange* extension_range(int index) const;

        // Returns true if the number is in one of the extension ranges.
       [[nodiscard]]  bool IsExtensionNumber(int number) const;

        // Returns nullptr if no extension range contains the given number.
        [[nodiscard]] const ExtensionRange* FindExtensionRangeContainingNumber(int number) const;

        // The number of extensions defined nested within this message type's scope.
        // See doc:
        // https://developers.google.com/protocol-buffers/docs/proto#nested-extensions
        //
        // Note that the extensions may be extending *other* messages.
        //
        // For example:
        // message M1 {
        //   extensions 1 to max;
        // }
        //
        // message M2 {
        //   extend M1 {
        //     optional int32 foo = 1;
        //   }
        // }
        //
        // In this case,
        // DescriptorPool::generated_pool()
        //     ->FindMessageTypeByName("M2")
        //     ->extension(0)
        // will return "foo", even though "foo" is an extension of M1.
        // To find all known extensions of a given message, instead use
        // DescriptorPool::FindAllExtensions.
        [[nodiscard]] int extension_count() const;
        // Get an extension by index, where 0 <= index < extension_count().
        // These are returned in the order they were defined in the .shark file.
        [[nodiscard]] const FieldDescriptor* extension(int index) const;

        // Looks up a named extension (which extends some *other* message type)
        // defined within this message type's scope.
        [[nodiscard]] const FieldDescriptor* FindExtensionByName(ConstStringParam name) const;

        // Similar to FindFieldByLowercaseName(), but finds extensions defined within
        // this message type's scope.
        [[nodiscard]] const FieldDescriptor* FindExtensionByLowercaseName(
            ConstStringParam name) const;

        // Similar to FindFieldByCamelcaseName(), but finds extensions defined within
        // this message type's scope.
        const FieldDescriptor* FindExtensionByCamelcaseName(
            ConstStringParam name) const;

        // Reserved fields -------------------------------------------------

        // A range of reserved field numbers.
        struct ReservedRange {
            int start; // inclusive
            int end; // exclusive
        };

        // The number of reserved ranges in this message type.
        [[nodiscard]] int reserved_range_count() const;
        // Gets an reserved range by index, where 0 <= index <
        // reserved_range_count(). These are returned in the order they were defined
        // in the .shark file.
       [[nodiscard]]  const ReservedRange* reserved_range(int index) const;

        // Returns true if the number is in one of the reserved ranges.
        [[nodiscard]] bool IsReservedNumber(int number) const;

        // Returns nullptr if no reserved range contains the given number.
        [[nodiscard]] const ReservedRange* FindReservedRangeContainingNumber(int number) const;

        // The number of reserved field names in this message type.
        [[nodiscard]] int reserved_name_count() const;

        // Gets a reserved name by index, where 0 <= index < reserved_name_count().
        [[nodiscard]] const std::string& reserved_name(int index) const;

        // Returns true if the field name is reserved.
        [[nodiscard]] bool IsReservedName(ConstStringParam name) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this message declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

        // Maps --------------------------------------------------------------

        // Returns the FieldDescriptor for the "key" field. If this isn't a map entry
        // field, returns nullptr.
        [[nodiscard]] const FieldDescriptor* map_key() const;

        // Returns the FieldDescriptor for the "value" field. If this isn't a map
        // entry field, returns nullptr.
       [[nodiscard]] const FieldDescriptor* map_value() const;

    private:
        friend class Symbol;
        typedef MessageOptions OptionsType;

        // Allows tests to test CopyTo(proto, true).
        friend class descriptor_unittest::DescriptorTest;

        // Allows access to GetLocationPath for annotations.
        friend class io::Printer;
        friend class compiler::cpp::Formatter;
        friend class compiler::cc::Formatter;

        // Fill the json_name field of FieldDescriptorProto.
        void CopyJsonNameTo(DescriptorProto* proto) const;

        // Internal version of DebugString; controls the level of indenting for
        // correct depth. Takes |options| to control debug-string options, and
        // |include_opening_clause| to indicate whether the "message ... " part of the
        // clause has already been generated (this varies depending on context).
        void DebugString(int depth, std::string* contents,
            const DebugStringOptions& options,
            bool include_opening_clause) const;

        // Walks up the descriptor tree to generate the source location path
        // to this descriptor from the file root.
        void GetLocationPath(std::vector<int>* output) const;

        // True if this is a placeholder for an unknown type.
        bool is_placeholder_ : 1;
        // True if this is a placeholder and the type name wasn't fully-qualified.
        bool is_unqualified_placeholder_ : 1;
        // Well known type.  Stored like this to conserve space.
        uint8_t well_known_type_ : 5;

        // This points to the last field _number_ that is part of the sequence
        // starting at 1, where
        //     `desc->field(i)->number() == i + 1`
        // A value of `0` means no field matches. That is, there are no fields or the
        // first field is not field `1`.
        // Uses 16-bit to avoid extra padding. Unlikely to have more than 2^16
        // sequentially numbered fields in a message.
        uint16_t sequential_field_limit_;

        int field_count_;

        // all_names_ = [name, full_name]
        const std::string* all_names_;
        const FileDescriptor* file_;
        const Descriptor* containing_type_;
        const MessageOptions* options_;

        // These arrays are separated from their sizes to minimize padding on 64-bit.
        FieldDescriptor* fields_;
        OneofDescriptor* oneof_decls_;
        Descriptor* nested_types_;
        EnumDescriptor* enum_types_;
        ExtensionRange* extension_ranges_;
        FieldDescriptor* extensions_;
        ReservedRange* reserved_ranges_;
        const std::string** reserved_names_;

        int oneof_decl_count_;
        int real_oneof_decl_count_;
        int nested_type_count_;
        int enum_type_count_;
        int extension_range_count_;
        int extension_count_;
        int reserved_range_count_;
        int reserved_name_count_;

        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<Descriptor>() and AllocateArray<Descriptor>() in descriptor.cc
        // and update them to initialize the field.

        // Must be constructed using DescriptorPool.
        Descriptor() = default;
        friend class DescriptorBuilder;
        friend class DescriptorPool;
        friend class EnumDescriptor;
        friend class FieldDescriptor;
        friend class FileDescriptorTables;
        friend class OneofDescriptor;
        friend class MethodDescriptor;
        friend class FileDescriptor;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Descriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(Descriptor, 136);

        // Describes a oneof defined in a message type.
    class SHARK_EXPORT OneofDescriptor : private internal::SymbolBase {
    public:
        typedef OneofDescriptorProto Proto;

        const std::string& name() const; // Name of this oneof.
        const std::string& full_name() const; // Fully-qualified name of the oneof.

        // Index of this oneof within the message's oneof array.
        int index() const;

        // Returns whether this oneof was inserted by the compiler to wrap a proto3
        // optional field. If this returns true, code generators should *not* emit it.
        bool is_synthetic() const;

        // The .shark file in which this oneof was defined.  Never nullptr.
        const FileDescriptor* file() const;
        // The Descriptor for the message containing this oneof.
        const Descriptor* containing_type() const;

        // The number of (non-extension) fields which are members of this oneof.
        int field_count() const;
        // Get a member of this oneof, in the order in which they were declared in the
        // .shark file.  Does not include extensions.
        const FieldDescriptor* field(int index) const;

        const OneofOptions& options() const;

        // See Descriptor::CopyTo().
        void CopyTo(OneofDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this oneof declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef OneofOptions OptionsType;

        // Allows access to GetLocationPath for annotations.
        friend class io::Printer;
        friend class compiler::cpp::Formatter;
        friend class compiler::cc::Formatter;

        // See Descriptor::DebugString().
        void DebugString(int depth, std::string* contents,
            const DebugStringOptions& options) const;

        // Walks up the descriptor tree to generate the source location path
        // to this descriptor from the file root.
        void GetLocationPath(std::vector<int>* output) const;

        int field_count_;

        // all_names_ = [name, full_name]
        const std::string* all_names_;
        const Descriptor* containing_type_;
        const OneofOptions* options_;
        const FieldDescriptor* fields_;

        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<OneofDescriptor>() and AllocateArray<OneofDescriptor>()
        // in descriptor.cc and update them to initialize the field.

        // Must be constructed using DescriptorPool.
        OneofDescriptor() { }
        friend class DescriptorBuilder;
        friend class Descriptor;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(OneofDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(OneofDescriptor, 40);

}  // namespace shark

#include <shark/port_undef.inc>

