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

        // Describes a whole .shark file.  To get the FileDescriptor for a compiled-in
    // file, get the descriptor for something defined in that file and call
    // descriptor->file().  Use DescriptorPool to construct your own descriptors.
    class SHARK_EXPORT FileDescriptor : private internal::SymbolBase {
    public:
        typedef FileDescriptorProto Proto;

        // The filename, relative to the source tree.
        // e.g. "foo/bar/baz.shark"
        const std::string& name() const;

        // The package, e.g. "shark.compiler".
        const std::string& package() const;

        // The DescriptorPool in which this FileDescriptor and all its contents were
        // allocated.  Never nullptr.
        const DescriptorPool* pool() const;

        // The number of files imported by this one.
        int dependency_count() const;
        // Gets an imported file by index, where 0 <= index < dependency_count().
        // These are returned in the order they were defined in the .shark file.
        const FileDescriptor* dependency(int index) const;

        // The number of files public imported by this one.
        // The public dependency list is a subset of the dependency list.
        int public_dependency_count() const;
        // Gets a public imported file by index, where 0 <= index <
        // public_dependency_count().
        // These are returned in the order they were defined in the .shark file.
        const FileDescriptor* public_dependency(int index) const;

        // The number of files that are imported for weak fields.
        // The weak dependency list is a subset of the dependency list.
        int weak_dependency_count() const;
        // Gets a weak imported file by index, where 0 <= index <
        // weak_dependency_count().
        // These are returned in the order they were defined in the .shark file.
        const FileDescriptor* weak_dependency(int index) const;

        // Number of top-level message types defined in this file.  (This does not
        // include nested types.)
        int message_type_count() const;
        // Gets a top-level message type, where 0 <= index < message_type_count().
        // These are returned in the order they were defined in the .shark file.
        const Descriptor* message_type(int index) const;

        // Number of top-level enum types defined in this file.  (This does not
        // include nested types.)
        int enum_type_count() const;
        // Gets a top-level enum type, where 0 <= index < enum_type_count().
        // These are returned in the order they were defined in the .shark file.
        const EnumDescriptor* enum_type(int index) const;

        // Number of services defined in this file.
        int service_count() const;
        // Gets a service, where 0 <= index < service_count().
        // These are returned in the order they were defined in the .shark file.
        const ServiceDescriptor* service(int index) const;

        // Number of extensions defined at file scope.  (This does not include
        // extensions nested within message types.)
        int extension_count() const;
        // Gets an extension's descriptor, where 0 <= index < extension_count().
        // These are returned in the order they were defined in the .shark file.
        const FieldDescriptor* extension(int index) const;

        // Get options for this file.  These are specified in the .shark file by
        // placing lines like "option foo = 1234;" at the top level, outside of any
        // other definitions.  Allowed options are defined by FileOptions in
        // descriptor.shark, and any available extensions of that message.
        const FileOptions& options() const;

        SyntaxEdition syntax() const;
        static const char* SyntaxName(SyntaxEdition syntax);

        // Find a top-level message type by name (not full_name).  Returns nullptr if
        // not found.
        const Descriptor* FindMessageTypeByName(ConstStringParam name) const;
        // Find a top-level enum type by name.  Returns nullptr if not found.
        const EnumDescriptor* FindEnumTypeByName(ConstStringParam name) const;
        // Find an enum value defined in any top-level enum by name.  Returns nullptr
        // if not found.
        const EnumValueDescriptor* FindEnumValueByName(ConstStringParam name) const;
        // Find a service definition by name.  Returns nullptr if not found.
        const ServiceDescriptor* FindServiceByName(ConstStringParam name) const;
        // Find a top-level extension definition by name.  Returns nullptr if not
        // found.
        const FieldDescriptor* FindExtensionByName(ConstStringParam name) const;
        // Similar to FindExtensionByName(), but searches by lowercased-name.  See
        // Descriptor::FindFieldByLowercaseName().
        const FieldDescriptor* FindExtensionByLowercaseName(
            ConstStringParam name) const;
        // Similar to FindExtensionByName(), but searches by camelcased-name.  See
        // Descriptor::FindFieldByCamelcaseName().
        const FieldDescriptor* FindExtensionByCamelcaseName(
            ConstStringParam name) const;

        // See Descriptor::CopyTo().
        // Notes:
        // - This method does NOT copy source code information since it is relatively
        //   large and rarely needed.  See CopySourceCodeInfoTo() below.
        void CopyTo(FileDescriptorProto* proto) const;
        // Write the source code information of this FileDescriptor into the given
        // FileDescriptorProto.  See CopyTo() above.
        void CopySourceCodeInfoTo(FileDescriptorProto* proto) const;
        // Fill the json_name field of FieldDescriptorProto for all fields. Can only
        // be called after CopyTo().
        void CopyJsonNameTo(FileDescriptorProto* proto) const;

        // See Descriptor::DebugString().
        std::string DebugString() const;

        // See Descriptor::DebugStringWithOptions().
        std::string DebugStringWithOptions(const DebugStringOptions& options) const;

        // Returns true if this is a placeholder for an unknown file. This will
        // only be the case if this descriptor comes from a DescriptorPool
        // with AllowUnknownDependencies() set.
        bool is_placeholder() const;

        // Updates |*out_location| to the source location of the complete extent of
        // this file declaration (namely, the empty path).
        bool GetSourceLocation(SourceLocation* out_location) const;

        // Updates |*out_location| to the source location of the complete
        // extent of the declaration or declaration-part denoted by |path|.
        // Returns false and leaves |*out_location| unchanged iff location
        // information was not available.  (See SourceCodeInfo for
        // description of path encoding.)
        bool GetSourceLocation(const std::vector<int>& path,
            SourceLocation* out_location) const;

    private:
        friend class Symbol;
        typedef FileOptions OptionsType;

        bool is_placeholder_;
        // Indicates the FileDescriptor is completed building. Used to verify
        // that type accessor functions that can possibly build a dependent file
        // aren't called during the process of building the file.
        bool finished_building_;
        // Actually a `Syntax` but stored as uint8_t to save space.
        SyntaxEdition syntax_;
        // This one is here to fill the padding.
        int extension_count_;

        const std::string* name_;
        const std::string* package_;
        const DescriptorPool* pool_;

        // dependencies_once_ contain a once_flag followed by N NUL terminated
        // strings. Dependencies that do not need to be loaded will be empty. ie just
        // {'\0'}
        internal::once_flag* dependencies_once_;
        static void DependenciesOnceInit(const FileDescriptor* to_init);
        void InternalDependenciesOnceInit() const;

        // These are arranged to minimize padding on 64-bit.
        int dependency_count_;
        int public_dependency_count_;
        int weak_dependency_count_;
        int message_type_count_;
        int enum_type_count_;
        int service_count_;

        mutable const FileDescriptor** dependencies_;
        int* public_dependencies_;
        int* weak_dependencies_;
        Descriptor* message_types_;
        EnumDescriptor* enum_types_;
        ServiceDescriptor* services_;
        FieldDescriptor* extensions_;
        const FileOptions* options_;

        const FileDescriptorTables* tables_;
        const SourceCodeInfo* source_code_info_;

        // IMPORTANT:  If you add a new field, make sure to search for all instances
        // of Allocate<FileDescriptor>() and AllocateArray<FileDescriptor>() in
        // descriptor.cc and update them to initialize the field.

        FileDescriptor() { }
        friend class DescriptorBuilder;
        friend class DescriptorPool;
        friend class Descriptor;
        friend class FieldDescriptor;
        friend class internal::LazyDescriptor;
        friend class OneofDescriptor;
        friend class EnumDescriptor;
        friend class EnumValueDescriptor;
        friend class MethodDescriptor;
        friend class ServiceDescriptor;
        SHARK_DISALLOW_EVIL_CONSTRUCTORS(FileDescriptor);
    };

    SHARK_INTERNAL_CHECK_CLASS_SIZE(FileDescriptor, 144);

} // namespace shark


#include <shark/port_undef.inc>
