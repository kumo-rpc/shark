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

#include <turbo/strings/str_cat.h>

#include <shark/schema/internal/descriptor_builder.h>
#include <shark/schema/internal/file_descriptor_tables_inl.h>
#include <turbo/strings/charconv.h>
#include <turbo/strings/escaping.h>
#include <turbo/strings/substitute.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // These plan_allocation_size functions will gather into the FlatAllocator all the
    // necessary memory allocations that BuildXXX functions below will do on the
    // Tables object.
    // They *must* be kept in sync. If we miss some PlanArray call we won't have
    // enough memory and will SHARK_CHECK-fail.
    inline  void plan_allocation_size(
        const RepeatedPtrField<EnumValueDescriptorProto>& values,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<EnumValueDescriptor>(values.size());
        alloc.PlanArray<std::string>(2 * values.size()); // name + full_name
        for (const auto& v : values) {
            if (v.has_options())
                alloc.PlanArray<EnumValueOptions>(1);
        }
    }

    inline  void plan_allocation_size(
    const RepeatedPtrField<EnumDescriptorProto>& enums,
    internal::FlatAllocator& alloc) {
        alloc.PlanArray<EnumDescriptor>(enums.size());
        alloc.PlanArray<std::string>(2 * enums.size()); // name + full_name
        for (const auto& e : enums) {
            if (e.has_options())
                alloc.PlanArray<EnumOptions>(1);
            plan_allocation_size(e.value(), alloc);
            alloc.PlanArray<EnumDescriptor::ReservedRange>(e.reserved_range_size());
            alloc.PlanArray<const std::string*>(e.reserved_name_size());
            alloc.PlanArray<std::string>(e.reserved_name_size());
        }
    }

    inline  void plan_allocation_size(
        const RepeatedPtrField<OneofDescriptorProto>& oneofs,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<OneofDescriptor>(oneofs.size());
        alloc.PlanArray<std::string>(2 * oneofs.size()); // name + full_name
        for (const auto& oneof : oneofs) {
            if (oneof.has_options())
                alloc.PlanArray<OneofOptions>(1);
        }
    }

    inline  void plan_allocation_size(
        const RepeatedPtrField<FieldDescriptorProto>& fields,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<FieldDescriptor>(fields.size());
        for (const auto& field : fields) {
            if (field.has_options())
                alloc.PlanArray<FieldOptions>(1);
            alloc.PlanFieldNames(field.name(),
                field.has_json_name() ? &field.json_name() : nullptr);
            if (field.has_default_value() && field.has_type() && (field.type() == SharkType::STRING || field.type() == SharkType::BYTES)) {
                // For the default string value.
                alloc.PlanArray<std::string>(1);
            }
        }
    }

    inline  void plan_allocation_size(
        const RepeatedPtrField<DescriptorProto::ExtensionRange>& ranges,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<Descriptor::ExtensionRange>(ranges.size());
        for (const auto& r : ranges) {
            if (r.has_options())
                alloc.PlanArray<ExtensionRangeOptions>(1);
        }
    }


    inline  void plan_allocation_size(
        const RepeatedPtrField<DescriptorProto>& messages,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<Descriptor>(messages.size());
        alloc.PlanArray<std::string>(2 * messages.size()); // name + full_name

        for (const auto& message : messages) {
            if (message.has_options())
                alloc.PlanArray<MessageOptions>(1);
            plan_allocation_size(message.nested_type(), alloc);
            plan_allocation_size(message.field(), alloc);
            plan_allocation_size(message.anonymous_extension(), alloc);
            plan_allocation_size(message.extension_range(), alloc);
            alloc.PlanArray<Descriptor::ReservedRange>(message.reserved_range_size());
            alloc.PlanArray<const std::string*>(message.reserved_name_size());
            alloc.PlanArray<std::string>(message.reserved_name_size());
            plan_allocation_size(message.enum_type(), alloc);
            plan_allocation_size(message.oneof_decl(), alloc);
        }
    }

    inline  void plan_allocation_size(
        const RepeatedPtrField<MethodDescriptorProto>& methods,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<MethodDescriptor>(methods.size());
        alloc.PlanArray<std::string>(2 * methods.size()); // name + full_name
        for (const auto& m : methods) {
            if (m.has_options())
                alloc.PlanArray<MethodOptions>(1);
        }
    }

    inline  void plan_allocation_size(
        const RepeatedPtrField<ServletDescriptorProto>& servlets,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<ServletDescriptor>(servlets.size());
        alloc.PlanArray<std::string>(2 * servlets.size()); // name + full_name
        for (const auto& servlet : servlets) {
            if (servlet.has_options())
                alloc.PlanArray<ServletOptions>(1);
            plan_allocation_size(servlet.method(), alloc);
        }
    }

    inline  void plan_allocation_size(const FileDescriptorProto& proto,
        internal::FlatAllocator& alloc) {
        alloc.PlanArray<FileDescriptor>(1);
        alloc.PlanArray<FileDescriptorTables>(1);
        alloc.PlanArray<std::string>(2); // name + package
        if (proto.has_options())
            alloc.PlanArray<FileOptions>(1);
        if (proto.has_source_code_info())
            alloc.PlanArray<SourceCodeInfo>(1);

        plan_allocation_size(proto.servlet(), alloc);
        plan_allocation_size(proto.message_type(), alloc);
        plan_allocation_size(proto.enum_type(), alloc);
        plan_allocation_size(proto.anonymous_extension(), alloc);

        alloc.PlanArray<int>(proto.weak_dependency_size());
        alloc.PlanArray<int>(proto.public_dependency_size());
        alloc.PlanArray<const FileDescriptor*>(proto.dependency_size());
    }

    // Allocates an array of two strings, the first one is a copy of `proto_name`,
    // and the second one is the full name.
    // Full proto name is "scope.proto_name" if scope is non-empty and
    // "proto_name" otherwise.
    inline const std::string* allocate_name_strings(const std::string& scope,
        const std::string& proto_name, internal::FlatAllocator& alloc) {
        if (scope.empty()) {
            return alloc.AllocateStrings(proto_name, proto_name);
        } else {
            return alloc.AllocateStrings(proto_name,
                turbo::str_cat(scope, ".", proto_name));
        }
    }
} // namespace shark


#include <shark/port_undef.inc>
