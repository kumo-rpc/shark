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
#include <string>
#include <vector>
#include <shark/schema/fwd.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // Describes a type of shark message  To obtain the Descriptor for
    // a given message object, call Message::get_descriptor().  Generated
    // message classes also have a static method called descriptor()
    // which returns the type's descriptor.
    // Use DescriptorPool to construct your own descriptors.
    class SHARK_EXPORT TypeDescriptor : private internal::DescriptorIdBase {
    public:
        // The name of the message type, not including its scope.
        const std::string& name() const;

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
        [[nodiscard]] const TypeFileDescriptor* file() const;

        // If this Descriptor describes a nested type, this returns the type
        // in which it is nested.  Otherwise, returns nullptr.
        [[nodiscard]] const TypeDescriptor* containing_type() const;

        // Get options for this message type.  These are specified in the .shark file
        // by placing lines like "option foo = 1234;" in the message definition.
        // Allowed options are defined by MessageOptions in descriptor.shark, and any
        // available extensions of that message.
        [[nodiscard]] const MessageOptions& options() const;

        // Write the contents of this Descriptor into the given DescriptorProto.
        // The target DescriptorProto must be clear before calling this; if it
        // isn't, the result may be garbage.
        // void CopyTo(DescriptorProto* proto) const;

        // Write the contents of this descriptor in a human-readable form. Output
        // will be suitable for re-parsing.
        [[nodiscard]] std::string debug_string() const;

        // Similar to DebugString(), but additionally takes options (e.g.,
        // include original user comments in output).
        [[nodiscard]] std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Returns true if this is a placeholder for an unknown type. This will
        // only be the case if this descriptor comes from a DescriptorPool
        // with AllowUnknownDependencies() set.
        [[nodiscard]] bool is_placeholder() const;

        // Field stuff -----------------------------------------------------

        // The number of fields in this message type.
        [[nodiscard]] int field_count() const;
        // Gets a field by index, where 0 <= index < field_count().
        // These are returned in the order they were defined in the .shark file.
        [[nodiscard]] const TypeFieldDescriptor* field(int index) const;

        // Looks up a field by declared tag number.  Returns nullptr if no such field
        // exists.
        [[nodiscard]] const TypeFieldDescriptor* FindFieldByNumber(int number) const;
        // Looks up a field by name.  Returns nullptr if no such field exists.
        [[nodiscard]] const TypeFieldDescriptor* FindFieldByName(std::string_view name) const;

        // Looks up a field by lowercased name (as returned by lowercase_name()).
        // This lookup may be ambiguous if multiple field names differ only by case,
        // in which case the field returned is chosen arbitrarily from the matches.
        [[nodiscard]] const TypeFieldDescriptor* FindFieldByLowercaseName(std::string_view lowercase_name) const;

        // Looks up a field by camel-case name (as returned by camelcase_name()).
        // This lookup may be ambiguous if multiple field names differ in a way that
        // leads them to have identical camel-case names, in which case the field
        // returned is chosen arbitrarily from the matches.
        [[nodiscard]] const TypeFieldDescriptor* FindFieldByCamelcaseName(std::string_view camelcase_name) const;

        // The number of oneofs in this message type.
       [[nodiscard]] int oneof_decl_count() const;
        // The number of oneofs in this message type, excluding synthetic oneofs.
        // Real oneofs always come first, so iterating up to real_oneof_decl_cout()
        // will yield all real oneofs.
       [[nodiscard]] int real_oneof_decl_count() const;
        // Get a oneof by index, where 0 <= index < oneof_decl_count().
        // These are returned in the order they were defined in the .shark file.
       [[nodiscard]]  const OneofDescriptor* oneof_decl(int index) const;

        // Looks up a oneof by name.  Returns nullptr if no such oneof exists.
       [[nodiscard]] const OneofDescriptor* FindOneofByName(std::string_view name) const;

        // Nested type stuff -----------------------------------------------

        // The number of nested types in this message type.
      [[nodiscard]]  int nested_type_count() const;
        // Gets a nested type by index, where 0 <= index < nested_type_count().
        // These are returned in the order they were defined in the .shark file.
       [[nodiscard]] const TypeDescriptor* nested_type(int index) const;

        // Looks up a nested type by name.  Returns nullptr if no such nested type
        // exists.
       [[nodiscard]] const TypeDescriptor* FindNestedTypeByName(std::string_view name) const;

        // Enum stuff ------------------------------------------------------

        // The number of enum types in this message type.
        int enum_type_count() const;
        // Gets an enum type by index, where 0 <= index < enum_type_count().
        // These are returned in the order they were defined in the .shark file.
       [[nodiscard]] const TypeEnumDescriptor* enum_type(int index) const;

        // Looks up an enum type by name.  Returns nullptr if no such enum type
        // exists.
       [[nodiscard]] const TypeEnumDescriptor* FindEnumTypeByName(std::string_view name) const;

        // Looks up an enum value by name, among all enum types in this message.
        // Returns nullptr if no such value exists.
       [[nodiscard]] const TypeEnumDescriptor* FindEnumValueByName(std::string_view name) const;

        // Reserved fields -------------------------------------------------

        // A range of reserved field numbers.
        struct ReservedRange {
            int start; // inclusive
            int end; // exclusive
        };

        // The number of reserved ranges in this message type.
        int reserved_range_count() const;
        // Gets an reserved range by index, where 0 <= index <
        // reserved_range_count(). These are returned in the order they were defined
        // in the .shark file.
        const ReservedRange* reserved_range(int index) const;

        // Returns true if the number is in one of the reserved ranges.
        bool IsReservedNumber(int number) const;

        // Returns nullptr if no reserved range contains the given number.
        const ReservedRange* FindReservedRangeContainingNumber(int number) const;

        // The number of reserved field names in this message type.
        int reserved_name_count() const;

        // Gets a reserved name by index, where 0 <= index < reserved_name_count().
        const std::string& reserved_name(int index) const;

        // Returns true if the field name is reserved.
        bool IsReservedName(std::string_view name) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this message declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;


    private:
        friend class Symbol;
        typedef MessageOptions OptionsType;

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
        const TypeFileDescriptor* file_;
        const TypeDescriptor* containing_type_;
        const MessageOptions* options_;

        // These arrays are separated from their sizes to minimize padding on 64-bit.
        TypeFieldDescriptor* fields_;
        TypeOneofDescriptor* oneof_decls_;
        TypeDescriptor* nested_types_;
        TypeEnumDescriptor* enum_types_;
        TypeFieldDescriptor* extensions_;
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

        TypeDescriptor() = default;
    };
}  // namespace shark

#include <shark/port_undef.inc>
