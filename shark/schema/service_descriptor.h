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

     // Describes an RPC service. Use DescriptorPool to construct your own
    // descriptors.
    class SHARK_EXPORT ServiceDescriptor : private internal::SymbolBase {
    public:
        typedef ServiceDescriptorProto Proto;

        // The name of the service, not including its containing scope.
        const std::string& name() const;
        // The fully-qualified name of the service, scope delimited by periods.
        const std::string& full_name() const;
        // Index of this service within the file's services array.
        int index() const;

        // The .shark file in which this service was defined.  Never nullptr.
        const FileDescriptor* file() const;

        // Get options for this service type.  These are specified in the .shark file
        // by placing lines like "option foo = 1234;" in the service definition.
        // Allowed options are defined by ServiceOptions in descriptor.shark, and any
        // available extensions of that message.
        const ServiceOptions& options() const;

        // The number of methods this service defines.
        int method_count() const;
        // Gets a MethodDescriptor by index, where 0 <= index < method_count().
        // These are returned in the order they were defined in the .shark file.
        const MethodDescriptor* method(int index) const;

        // Look up a MethodDescriptor by name.
        const MethodDescriptor* FindMethodByName(ConstStringParam name) const;

        // See Descriptor::CopyTo().
        void CopyTo(ServiceDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this service declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef ServiceOptions OptionsType;

        // Allows access to GetLocationPath for annotations.
        friend class io::Printer;
        friend class compiler::cpp::Formatter;
        friend class compiler::cc::Formatter;

        // See Descriptor::DebugString().
        void DebugString(std::string* contents,
            const DebugStringOptions& options) const;

        // Walks up the descriptor tree to generate the source location path
        // to this descriptor from the file root.
        void GetLocationPath(std::vector<int>* output) const;

        // all_names_ = [name, full_name]
        const std::string* all_names_;
        const FileDescriptor* file_;
        const ServiceOptions* options_;
        MethodDescriptor* methods_;
        int method_count_;
        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<ServiceDescriptor>() and AllocateArray<ServiceDescriptor>() in
        // descriptor.cc and update them to initialize the field.

        // Must be constructed using DescriptorPool.
        ServiceDescriptor() { }
        friend class DescriptorBuilder;
        friend class FileDescriptor;
        friend class MethodDescriptor;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ServiceDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(ServiceDescriptor, 48);

     // Describes an individual service method.  To obtain a MethodDescriptor given
    // a service, first get its ServiceDescriptor, then call
    // ServiceDescriptor::FindMethodByName().  Use DescriptorPool to construct your
    // own descriptors.
    class SHARK_EXPORT MethodDescriptor : private internal::SymbolBase {
    public:
        typedef MethodDescriptorProto Proto;

        // Name of this method, not including containing scope.
        const std::string& name() const;
        // The fully-qualified name of the method, scope delimited by periods.
        const std::string& full_name() const;
        // Index within the service's Descriptor.
        int index() const;

        // The .shark file in which this method was defined.  Never nullptr.
        const FileDescriptor* file() const;
        // Gets the service to which this method belongs.  Never nullptr.
        const ServiceDescriptor* service() const;

        // Gets the type of protocol message which this method accepts as input.
        const Descriptor* input_type() const;
        // Gets the type of protocol message which this message produces as output.
        const Descriptor* output_type() const;

        // Gets whether the client streams multiple requests.
        bool client_streaming() const;
        // Gets whether the server streams multiple responses.
        bool server_streaming() const;

        // Get options for this method.  These are specified in the .shark file by
        // placing lines like "option foo = 1234;" in curly-braces after a method
        // declaration.  Allowed options are defined by MethodOptions in
        // descriptor.shark, and any available extensions of that message.
        const MethodOptions& options() const;

        // See Descriptor::CopyTo().
        void CopyTo(MethodDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Source Location ---------------------------------------------------

        // Updates |*out_location| to the source location of the complete
        // extent of this method declaration.  Returns false and leaves
        // |*out_location| unchanged iff location information was not available.
        bool GetSourceLocation(SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef MethodOptions OptionsType;

        // Allows access to GetLocationPath for annotations.
        friend class io::Printer;
        friend class compiler::cpp::Formatter;

        // See Descriptor::DebugString().
        void DebugString(int depth, std::string* contents,
            const DebugStringOptions& options) const;

        // Walks up the descriptor tree to generate the source location path
        // to this descriptor from the file root.
        void GetLocationPath(std::vector<int>* output) const;

        bool client_streaming_;
        bool server_streaming_;
        // all_names_ = [name, full_name]
        const std::string* all_names_;
        const ServiceDescriptor* service_;
        mutable internal::LazyDescriptor input_type_;
        mutable internal::LazyDescriptor output_type_;
        const MethodOptions* options_;
        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<MethodDescriptor>() and AllocateArray<MethodDescriptor>() in
        // descriptor.cc and update them to initialize the field.

        // Must be constructed using DescriptorPool.
        MethodDescriptor() { }
        friend class DescriptorBuilder;
        friend class ServiceDescriptor;
        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MethodDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(MethodDescriptor, 64);
}  // namespace shark

#include <shark/port_undef.inc>

