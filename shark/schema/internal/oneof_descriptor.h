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
#include <shark/schema/fwd.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

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
        SHARK_DISALLOW_EVIL_CONSTRUCTORS(OneofDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(OneofDescriptor, 40);

} // namespace shark


#include <shark/port_undef.inc>
