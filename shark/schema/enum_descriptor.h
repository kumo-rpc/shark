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

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

        // Describes an enum type defined in a .shark file.  To get the EnumDescriptor
    // for a generated enum type, call TypeName_descriptor().  Use DescriptorPool
    // to construct your own descriptors.
    class SHARK_EXPORT EnumDescriptor : private internal::SymbolBase {
    public:
        typedef EnumDescriptorProto Proto;

        // The name of this enum type in the containing scope.
        const std::string& name() const;

        // The fully-qualified name of the enum type, scope delimited by periods.
        const std::string& full_name() const;

        // Index of this enum within the file or containing message's enum array.
        int index() const;

        // The .shark file in which this enum type was defined.  Never nullptr.
        const FileDescriptor* file() const;

        // The number of values for this EnumDescriptor.  Guaranteed to be greater
        // than zero.
        int value_count() const;
        // Gets a value by index, where 0 <= index < value_count().
        // These are returned in the order they were defined in the .shark file.
        const EnumValueDescriptor* value(int index) const;

        // Looks up a value by name.  Returns nullptr if no such value exists.
        const EnumValueDescriptor* FindValueByName(ConstStringParam name) const;
        // Looks up a value by number.  Returns nullptr if no such value exists.  If
        // multiple values have this number, the first one defined is returned.
        const EnumValueDescriptor* FindValueByNumber(int number) const;

        // If this enum type is nested in a message type, this is that message type.
        // Otherwise, nullptr.
        const Descriptor* containing_type() const;

        // Get options for this enum type.  These are specified in the .shark file by
        // placing lines like "option foo = 1234;" in the enum definition.  Allowed
        // options are defined by EnumOptions in descriptor.shark, and any available
        // extensions of that message.
        const EnumOptions& options() const;

        // See Descriptor::CopyTo().
        void CopyTo(EnumDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Returns true if this is a placeholder for an unknown enum. This will
        // only be the case if this descriptor comes from a DescriptorPool
        // with AllowUnknownDependencies() set.
        bool is_placeholder() const;

        // Reserved fields -------------------------------------------------

        // A range of reserved field numbers.
        struct ReservedRange {
            int start; // inclusive
            int end; // inclusive
        };

        // The number of reserved ranges in this message type.
        int reserved_range_count() const;
        // Gets an reserved range by index, where 0 <= index <
        // reserved_range_count(). These are returned in the order they were defined
        // in the .shark file.
        const EnumDescriptor::ReservedRange* reserved_range(int index) const;

        // Returns true if the number is in one of the reserved ranges.
        bool IsReservedNumber(int number) const;

        // Returns nullptr if no reserved range contains the given number.
        const EnumDescriptor::ReservedRange* FindReservedRangeContainingNumber(
            int number) const;

        // The number of reserved field names in this message type.
        int reserved_name_count() const;

        // Gets a reserved name by index, where 0 <= index < reserved_name_count().
        const std::string& reserved_name(int index) const;

        // Returns true if the field name is reserved.
        bool IsReservedName(ConstStringParam name) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this enum declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef EnumOptions OptionsType;

        // Allows access to GetLocationPath for annotations.
        friend class io::Printer;
        friend class compiler::cpp::Formatter;
        friend class compiler::cc::Formatter;

        // Allow access to FindValueByNumberCreatingIfUnknown.
        friend class descriptor_unittest::DescriptorTest;

        // Looks up a value by number.  If the value does not exist, dynamically
        // creates a new EnumValueDescriptor for that value, assuming that it was
        // unknown. If a new descriptor is created, this is done in a thread-safe way,
        // and future calls will return the same value descriptor pointer.
        //
        // This is private but is used by Reflection (which is friended below) to
        // return a valid EnumValueDescriptor from GetEnum() when this feature is
        // enabled.
        const EnumValueDescriptor* FindValueByNumberCreatingIfUnknown(
            int number) const;

        // See Descriptor::DebugString().
        void DebugString(int depth, std::string* contents,
            const DebugStringOptions& options) const;

        // Walks up the descriptor tree to generate the source location path
        // to this descriptor from the file root.
        void GetLocationPath(std::vector<int>* output) const;

        // True if this is a placeholder for an unknown type.
        bool is_placeholder_ : 1;
        // True if this is a placeholder and the type name wasn't fully-qualified.
        bool is_unqualified_placeholder_ : 1;

        // This points to the last value _index_ that is part of the sequence starting
        // with the first label, where
        //   `enum->value(i)->number() == enum->value(0)->number() + i`
        // We measure relative to the first label to adapt to enum labels starting at
        // 0 or 1.
        // Uses 16-bit to avoid extra padding. Unlikely to have more than 2^15
        // sequentially numbered labels in an enum.
        int16_t sequential_value_limit_;

        int value_count_;

        // all_names_ = [name, full_name]
        const std::string* all_names_;
        const FileDescriptor* file_;
        const Descriptor* containing_type_;
        const EnumOptions* options_;
        EnumValueDescriptor* values_;

        int reserved_range_count_;
        int reserved_name_count_;
        EnumDescriptor::ReservedRange* reserved_ranges_;
        const std::string** reserved_names_;

        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<EnumDescriptor>() and AllocateArray<EnumDescriptor>() in
        // descriptor.cc and update them to initialize the field.

        // Must be constructed using DescriptorPool.
        EnumDescriptor() { }
        friend class DescriptorBuilder;
        friend class Descriptor;
        friend class FieldDescriptor;
        friend class FileDescriptorTables;
        friend class EnumValueDescriptor;
        friend class FileDescriptor;
        friend class DescriptorPool;
        friend class Reflection;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(EnumDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(EnumDescriptor, 72);

    // Describes an individual enum constant of a particular type.  To get the
    // EnumValueDescriptor for a given enum value, first get the EnumDescriptor
    // for its type, then use EnumDescriptor::FindValueByName() or
    // EnumDescriptor::FindValueByNumber().  Use DescriptorPool to construct
    // your own descriptors.
    class SHARK_EXPORT EnumValueDescriptor : private internal::SymbolBaseN<0>,
                                             private internal::SymbolBaseN<1> {
    public:
        typedef EnumValueDescriptorProto Proto;

        const std::string& name() const; // Name of this enum constant.
        int index() const; // Index within the enums's Descriptor.
        int number() const; // Numeric value of this enum constant.

        // The full_name of an enum value is a sibling symbol of the enum type.
        // e.g. the full name of FieldDescriptorProto::TYPE_INT32 is actually
        // "shark.FieldDescriptorProto.TYPE_INT32", NOT
        // "shark.FieldDescriptorProto.Type.TYPE_INT32".  This is to conform
        // with C++ scoping rules for enums.
        const std::string& full_name() const;

        // The .shark file in which this value was defined.  Never nullptr.
        const FileDescriptor* file() const;
        // The type of this value.  Never nullptr.
        const EnumDescriptor* type() const;

        // Get options for this enum value.  These are specified in the .shark file by
        // adding text like "[foo = 1234]" after an enum value definition.  Allowed
        // options are defined by EnumValueOptions in descriptor.shark, and any
        // available extensions of that message.
        const EnumValueOptions& options() const;

        // See Descriptor::CopyTo().
        void CopyTo(EnumValueDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this enum value declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef EnumValueOptions OptionsType;

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

        int number_;
        // all_names_ = [name, full_name]
        const std::string* all_names_;
        const EnumDescriptor* type_;
        const EnumValueOptions* options_;
        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<EnumValueDescriptor>() and AllocateArray<EnumValueDescriptor>()
        // in descriptor.cc and update them to initialize the field.

        // Must be constructed using DescriptorPool.
        EnumValueDescriptor() { }
        friend class DescriptorBuilder;
        friend class EnumDescriptor;
        friend class DescriptorPool;
        friend class FileDescriptorTables;
        friend class Reflection;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(EnumValueDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(EnumValueDescriptor, 32);
}  // namespace shark

#include <shark/port_undef.inc>
