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

#include <shark/stubs/port.h>
#include <shark/stubs/once.h>
#include <shark/meta.h>
// Must be included last.
#include <shark/port_def.inc>

namespace shark {


    // Defined in this file.
    class Descriptor;
    class FieldDescriptor;
    class OneofDescriptor;
    class EnumDescriptor;
    class EnumValueDescriptor;
    class ServiceDescriptor;
    class MethodDescriptor;
    class FileDescriptor;
    class DescriptorDatabase;
    class DescriptorPool;

    // Defined in descriptor.shark
    class DescriptorProto;
    class DescriptorProto_ExtensionRange;
    class FieldDescriptorProto;
    class OneofDescriptorProto;
    class EnumDescriptorProto;
    class EnumValueDescriptorProto;
    class ServiceDescriptorProto;
    class MethodDescriptorProto;
    class FileDescriptorProto;
    class MessageOptions;
    class FieldOptions;
    class OneofOptions;
    class EnumOptions;
    class EnumValueOptions;
    class ExtensionRangeOptions;
    class ServiceOptions;
    class MethodOptions;
    class FileOptions;
    class UninterpretedOption;
    class SourceCodeInfo;

    // Defined in message.h
    class Message;
    class Reflection;

    // Defined in descriptor.cc
    class DescriptorBuilder;
    class FileDescriptorTables;
    class Symbol;

    // Defined in unknown_field_set.h.
    class UnknownField;

    // Defined in command_line_interface.cc
    namespace compiler {
        class CommandLineInterface;
        namespace cpp {
            // Defined in helpers.h
            class Formatter;
        } // namespace cpp
        namespace cc {
            // Defined in helpers.h
            class Formatter;
        } // namespace cpp
    } // namespace compiler

    namespace descriptor_unittest {
        class DescriptorTest;
    } // namespace descriptor_unittest

    // Defined in printer.h
    namespace io {
        class Printer;
    } // namespace io


}

// A class to handle the simplest cases of a lazily linked descriptor
// for a message type that isn't built at the time of cross linking,
// which is needed when a pool has lazily_build_dependencies_ set.
// Must be instantiated as mutable in a descriptor.

namespace shark::internal {

     // The classes in this file represent a significant memory footprint for the
        // library. We make sure we are not accidentally making them larger by
        // hardcoding the struct size for a specific platform. Use as:
        //
        //   SHARK_INTERNAL_CHECK_CLASS_SIZE(type, expected_size_in_x84-64);
        //

#if !defined(SHARK_INTERNAL_CHECK_CLASS_SIZE)
#define SHARK_INTERNAL_CHECK_CLASS_SIZE(t, expected)
#endif

        class FlatAllocator;

        class SHARK_EXPORT LazyDescriptor {
        public:
            // Init function to be called at init time of a descriptor containing
            // a LazyDescriptor.
            void Init() {
                descriptor_ = nullptr;
                once_ = nullptr;
            }

            // Sets the value of the descriptor if it is known during the descriptor
            // building process. Not thread safe, should only be called during the
            // descriptor build process. Should not be called after SetLazy has been
            // called.
            void Set(const Descriptor* descriptor);

            // Sets the information needed to lazily cross link the descriptor at a later
            // time, SetLazy is not thread safe, should be called only once at descriptor
            // build time if the symbol wasn't found and building of the file containing
            // that type is delayed because lazily_build_dependencies_ is set on the pool.
            // Should not be called after Set() has been called.
            void SetLazy(StringPiece name, const FileDescriptor* file);

            // Returns the current value of the descriptor, thread-safe. If SetLazy(...)
            // has been called, will do a one-time cross link of the type specified,
            // building the descriptor file that contains the type if necessary.
            inline const Descriptor* Get(const ServiceDescriptor* service) {
                Once(service);
                return descriptor_;
            }

        private:
            void Once(const ServiceDescriptor* service);

            const Descriptor* descriptor_;
            // The once_ flag is followed by a NUL terminated string for the type name.
            internal::once_flag* once_;
        };

    class SHARK_EXPORT SymbolBase {
    protected:
        friend class shark::Symbol;
        uint8_t symbol_type_;
    };

    // Some types have more than one SymbolBase because they have multiple
    // identities in the table. We can't have duplicate direct bases, so we use this
    // intermediate base to do so.
    // See BuildEnumValue for details.
    template <int N>
    class SHARK_EXPORT SymbolBaseN : public SymbolBase { };

}  // namespace shark::internal

namespace shark {

}
#include <shark/port_undef.inc>
